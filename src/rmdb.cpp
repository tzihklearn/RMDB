/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <cstdio>
#include <netinet/in.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <atomic>

#include "errors.h"
#include "optimizer/optimizer.h"
#include "recovery/log_recovery.h"
#include "optimizer/plan.h"
#include "optimizer/planner.h"
#include "portal.h"
#include "analyze/analyze.h"
#include "global_object/global_object.h"

#define SOCK_PORT 8765
#define MAX_CONN_LIMIT 8

static bool should_exit = false;


pthread_mutex_t *buffer_mutex;
pthread_mutex_t *sockfd_mutex;

static jmp_buf jmpbuf;

void sigint_handler(int signo) {
    should_exit = true;
//    log_manager->flush_buffer_to_disk();
    std::cout << "The Server receive Crtl+C, will been closed\n";
    longjmp(jmpbuf, 1);
}

// 判断当前正在执行的是显式事务还是单条SQL语句的事务，并更新事务ID
void SetTransaction(txn_id_t *txn_id, Context *context) {
    context->txn_ = txn_manager->get_transaction(*txn_id);
    if (context->txn_ == nullptr || context->txn_->get_state() == TransactionState::COMMITTED ||
        context->txn_->get_state() == TransactionState::ABORTED) {
        context->txn_ = txn_manager->begin(nullptr, context->log_mgr_);
        *txn_id = context->txn_->get_transaction_id();
        context->txn_->set_txn_mode(false);
    }
}

void *client_handler(void *sock_fd) {
    int fd = *((int *) sock_fd);
    pthread_mutex_unlock(sockfd_mutex);

    int i_recvBytes;
    // 接收客户端发送的请求
    char data_recv[BUFFER_LENGTH];
    // 需要返回给客户端的结果
    char *data_send = new char[BUFFER_LENGTH];
    // 需要返回给客户端的结果的长度
    int offset = 0;
    // 记录客户端当前正在执行的事务ID
    txn_id_t txn_id = INVALID_TXN_ID;

    std::string output = "establish client connection, sockfd: " + std::to_string(fd) + "\n";
    std::cout << output;
    JoinStrategy js = JoinStrategy();
    js.setSortMerge(true);
    while (true) {
        std::cout << "Waiting for request..." << std::endl;
        memset(data_recv, 0, BUFFER_LENGTH);

        i_recvBytes = read(fd, data_recv, BUFFER_LENGTH);

        if (i_recvBytes == 0) {
            std::cout << "Maybe the client has closed" << std::endl;
            break;
        }
        if (i_recvBytes == -1) {
            std::cout << "Client read error!" << std::endl;
            break;
        }

        printf("i_recvBytes: %d \n ", i_recvBytes);

        if (strcmp(data_recv, "exit") == 0) {
            std::cout << "Client exit." << std::endl;
            break;
        }
        if (strcmp(data_recv, "crash") == 0) {
            std::cout << "Server crash" << std::endl;
//            std::vector<int> asd;
////            asd[2] = 1;
//            std::fstream crash_outfile;
//            crash_outfile.open("crash.txt", std::ios::out | std::ios::app);
//            crash_outfile << "crash" << std::endl;
//            crash_outfile.close();
            exit(1);
        }

        std::cout << "Read from client " << fd << ": " << data_recv << std::endl;

        memset(data_send, '\0', BUFFER_LENGTH);
        offset = 0;

        // 开启事务，初始化系统所需的上下文信息（包括事务对象指针、锁管理器指针、日志管理器指针、存放结果的buffer、记录结果长度的变量）
        Context *context = new Context(lock_manager.get(), log_manager.get(), nullptr, &js, data_send, &offset);
        SetTransaction(&txn_id, context);

        // 用于判断是否已经调用了yy_delete_buffer来删除buf
        bool finish_analyze = false;
        pthread_mutex_lock(buffer_mutex);
        YY_BUFFER_STATE buf = yy_scan_string(data_recv);
        // yyparse()返回0表示语法解析成功
        if (yyparse() == 0) {
            if (ast::parse_tree != nullptr) {
                try {
                    // analyze and rewrite
                    std::shared_ptr<Query> query = analyze->do_analyze(ast::parse_tree, context);
                    yy_delete_buffer(buf);
                    finish_analyze = true;
                    pthread_mutex_unlock(buffer_mutex);
                    // 优化器
                    std::shared_ptr<Plan> plan = optimizer->plan_query(query, context);
                    // portal
                    std::shared_ptr<PortalStmt> portalStmt = portal->start(plan, context);
                    portal->run(portalStmt, ql_manager.get(), &txn_id, context);
                    portal->drop();
                } catch (TransactionAbortException &e) {
                    // 事务需要回滚，需要把abort信息返回给客户端并写入output.txt文件中
                    std::string str = "abort\n";
                    memcpy(data_send, str.c_str(), str.length());
                    data_send[str.length()] = '\0';
                    offset = str.length();

                    // 回滚事务
                    txn_manager->abort(context->txn_, log_manager.get());
                    std::cout << e.GetInfo() << std::endl;

                    try {
                        std::fstream outfile;
                        outfile.open("output.txt", std::ios::out | std::ios::app);
                        outfile << str;
                        outfile.close();
                    } catch (std::exception &e) {
                        std::cerr << "rmdb throw TransactionAbortException only pingcas can do" << e.what() << std::endl;
                    }
                } catch (RMDBError &e) {
                    // 遇到异常，需要打印failure到output.txt文件中，并发异常信息返回给客户端
                    std::cerr << e.what() << std::endl;

                    memcpy(data_send, e.what(), e.get_msg_len());
                    data_send[e.get_msg_len()] = '\n';
                    data_send[e.get_msg_len() + 1] = '\0';
                    offset = e.get_msg_len() + 1;

                    // 将报错信息写入output.txt
                    try {
                        std::fstream outfile;
                        outfile.open("output.txt",std::ios::out | std::ios::app);
                        outfile << "failure\n";
                        outfile.close();
                    } catch (std::exception &e) {
                        std::cerr << "rmdb throw RMDBERROR only pingcas can do" << e.what() << std::endl;
                    }
                }
            }
        } else {
            // 将报错信息写入output.txt
            try {
                std::fstream outfile;
                outfile.open("output.txt",std::ios::out | std::ios::app);
                outfile << "failure\n";
                outfile.close();
            } catch (std::exception &e) {
                std::cerr << "rmdb throw RMDBERROR only pingcas can do" << e.what() << std::endl;
            }
        }
        if (!finish_analyze) {
            yy_delete_buffer(buf);
            pthread_mutex_unlock(buffer_mutex);
        }
        // future TODO: 格式化 sql_handler.result, 传给客户端
        // send result with fixed format, use protobuf in the future
        if (write(fd, data_send, offset + 1) == -1) {
            break;
        }
        // 如果是单挑语句，需要按照一个完整的事务来执行，所以执行完当前语句后，自动提交事务
        if (!context->txn_->get_txn_mode()) {
            txn_manager->commit(context->txn_, context->log_mgr_);
        }
    }

    // Clear
    std::cout << "Terminating current client_connection..." << std::endl;
    close(fd);           // close a file descriptor.
    pthread_exit(nullptr);  // terminate calling thread!
}

void start_server() {
    // init mutex
    buffer_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    sockfd_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(buffer_mutex, nullptr);
    pthread_mutex_init(sockfd_mutex, nullptr);

    int sockfd_server;
    int fd_temp;
    struct sockaddr_in s_addr_in{};

    // 初始化连接
    sockfd_server = socket(AF_INET, SOCK_STREAM, 0);  // ipv4,TCP
    assert(sockfd_server != -1);
    int val = 1;
    setsockopt(sockfd_server, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // before bind(), set the attr of structure sockaddr.
    memset(&s_addr_in, 0, sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr_in.sin_port = htons(SOCK_PORT);
    fd_temp = bind(sockfd_server, (struct sockaddr *) (&s_addr_in), sizeof(s_addr_in));
    if (fd_temp == -1) {
        std::cout << "Bind error!" << std::endl;
        exit(1);
    }

    fd_temp = listen(sockfd_server, MAX_CONN_LIMIT);
    if (fd_temp == -1) {
        std::cout << "Listen error!" << std::endl;
        exit(1);
    }

    while (!should_exit) {
        std::cout << "Waiting for new connection..." << std::endl;
        pthread_t thread_id;
        struct sockaddr_in s_addr_client{};
        int client_length = sizeof(s_addr_client);

        if (setjmp(jmpbuf)) {
            std::cout << "Break from Server Listen Loop\n";
            break;
        }

        // Block here. Until server accepts a new connection.
        pthread_mutex_lock(sockfd_mutex);
        int sockfd = accept(sockfd_server, (struct sockaddr *) (&s_addr_client), (socklen_t *) (&client_length));
        if (sockfd == -1) {
            std::cout << "Accept error!" << std::endl;
            continue;  // ignore current socket ,continue while loop.
        }

        // 和客户端建立连接，并开启一个线程负责处理客户端请求
        if (pthread_create(&thread_id, nullptr, &client_handler, (void *) (&sockfd)) != 0) {
            std::cout << "Create thread fail!" << std::endl;
            break;  // break while loop
        }

    }

    // Clear
    std::cout << " Try to close all client-connection.\n";
    int ret = shutdown(sockfd_server, SHUT_WR);  // shut down the all or part of a full-duplex connection.
    if (ret == -1) { printf("%s\n", strerror(errno)); }
//    assert(ret != -1);
    sm_manager->close_db();
    std::cout << " DB has been closed.\n";
    std::cout << "Server shuts down." << std::endl;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        // 需要指定数据库名称
        std::cerr << "Usage: " << argv[0] << " <database>" << std::endl;
        exit(1);
    }

    signal(SIGINT, sigint_handler);
    try {
        std::cout << "\n"
                     "  _____  __  __ _____  ____  \n"
                     " |  __ \\|  \\/  |  __ \\|  _ \\ \n"
                     " | |__) | \\  / | |  | | |_) |\n"
                     " |  _  /| |\\/| | |  | |  _ < \n"
                     " | | \\ \\| |  | | |__| | |_) |\n"
                     " |_|  \\_\\_|  |_|_____/|____/ \n"
                     "\n"
                     "Welcome to RMDB!\n"
                     "Type 'help;' for help.\n"
                     "\n";
        // Database name is passed by args
        std::string db_name = argv[1];
        if (!sm_manager->is_dir(db_name)) {
            // Database not found, create a new one
            sm_manager->create_db(db_name);
        }

        // todo: 搞了checkpoint之后，这里的逻辑需要修改
//        std::this_thread::sleep_for(std::chrono::seconds(6));
        std::string crashPath = db_name + "/crash_my.txt";
        FILE *crashFile = fopen(crashPath.c_str(), "r");

        if (crashFile != nullptr) {
            remove(crashPath.c_str());
            std::string filePath = db_name + "/static_checkpoint_my.txt"; // 替换为您的文件路径
            FILE *file = fopen(filePath.c_str(), "r");
            if (file != nullptr) {
                fclose(file);
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(6));
            }
        } else {
            std::fstream crash_outfile;
            crash_outfile.open(crashPath, std::ios::out | std::ios::app);
            crash_outfile << "crash" << std::endl;
            crash_outfile.close();
        }


        // Open database
        sm_manager->open_db(db_name);

        // recovery database
//        int lsn = recovery->analyze();
//        log_manager->set_lsn(lsn);
        recovery->analyze();
        recovery->redo();
        recovery->undo();

        // 开启服务端，开始接受客户端连接
        start_server();
    } catch (RMDBError &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    return 0;
}
