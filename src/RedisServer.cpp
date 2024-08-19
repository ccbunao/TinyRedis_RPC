#include "RedisServer.h"

RedisServer *RedisServer::getInstance()
{
    static RedisServer redis;
    return &redis;
}

/**
 * 从指定的logo文件路径中读取内容，并打印到控制台。在读取过程中，会将文件中的"PORT"和"PTHREAD_ID"替换为实际的端口号和线程ID。
 *
 * @param logoFilePath 要读取的logo文件的路径。
 * @return 无返回值。
 */
void RedisServer::printLogo()
{
    std::ifstream ifs(logoFilePath);
    if (!ifs.is_open())
    {
        std::cout << "logoFilePath不存在" << std::endl;
    }
    std::string line = "";
    while (std::getline(ifs, line))
    {
        replaceText(line, "PORT", std::to_string(port));
        replaceText(line, "PTHREAD_ID", std::to_string(pid));
        std::cout << line << std::endl;
    }
}

/**
 * 打印服务器启动信息。
 *
 * @param startMessage 包含服务器启动信息的字符串，其中"PID"和"DATE"将被替换为实际的进程ID和日期。
 * @param initMessage 包含服务器准备接受连接的信息，其中"PORT"、"PID"和"DATE"将被替换为实际的端口号、进程ID和日期。
 * @return 无返回值。
 */
void RedisServer::printStartMessage()
{
    std::string startMessage = "[PID] DATE # Server started.";
    std::string initMessage = "[PID] DATE * The server is now ready to accept connections on port PORT";

    replaceText(startMessage, "PID", std::to_string(pid));
    replaceText(startMessage, "DATE", getDate());
    replaceText(initMessage, "PORT", std::to_string(port));
    replaceText(initMessage, "PID", std::to_string(pid));
    replaceText(initMessage, "DATE", getDate());

    std::cout << startMessage << std::endl;
    std::cout << initMessage << std::endl;
}

/**
 * 启动Redis服务器。
 *
 * @brief 该函数首先设置SIGINT信号的处理函数为signalHandler，然后打印出Redis的Logo和启动信息。
 * 注释部分是一个简单的客户端处理循环，从标准输入读取一行字符串，然后调用handleClient方法处理这行字符串，并将结果输出到标准输出。
 * 注意：这个循环会一直进行，直到stop变量被设置为true。
 */
void RedisServer::start()
{
    // 注册SIGINT信号处理函数，当接收到SIGINT信号时，会调用signalHandler函数进行处理
    signal(SIGINT, signalHandler);
    
    // 打印Redis服务器的Logo信息
    printLogo();
    
    // 打印启动消息
    printStartMessage();
    
    // 注释掉的代码段是一个循环，用于从标准输入读取字符串s，然后输出s和handleClient方法的返回值
    // string s ;
    // while (!stop) {
    //     // 从标准输入获取一行字符串并赋值给s
    //     getline(cin,s);
    //     
    //     // 输出获取到的字符串s
    //     cout << s <<endl;
    //     
    //     // 调用handleClient方法处理字符串s，并输出返回结果
    //     cout << RedisServer::handleClient(s) << endl;;
    // }
}

/**
 * 执行Redis服务器的事务操作。
 *
 * @param commandsQueue 存储待执行命令的队列。
 * @return 返回所有命令执行的结果，每个结果之间用换行符分隔。如果遇到"quit"或"exit"命令，则立即停止执行并返回"stop"。如果遇到"multi"命令，则打开事务，并提示"Open the transaction repeatedly!"。如果遇到"exec"命令而没有打开事务，则提示"No transaction is opened!"。对于其他命令，使用对应的解析器进行解析，并将解析结果添加到结果列表中。最后，将所有结果按顺序连接成一个字符串并返回。
 */
string RedisServer::executeTransaction(std::queue<std::string> &commandsQueue)
{
    // 存储所有的执行结果
    std::vector<std::string> responseMessagesList;
    while (!commandsQueue.empty())
    {
        std::string receivedData = std::move(commandsQueue.front());
        commandsQueue.pop();
        std::istringstream iss(receivedData);
        std::string command;
        std::vector<std::string> tokens;
        std::string responseMessage;

        while (iss >> command)
        {
            tokens.push_back(command);
        }
        command = tokens.front();
        if (!tokens.empty())
        {
            command = tokens.front();
            std::string responseMessage;
            if (command == "quit" || command == "exit")
            {
                responseMessage = "stop";
                return responseMessage;
            }
            else if (command == "multi")
            {
                responseMessage = "Open the transaction repeatedly!";
                responseMessagesList.emplace_back(responseMessage);
                continue;
            }
            else if (command == "exec")
            {
                // 处理未打开事物就执行的操作
                responseMessage = "No transaction is opened!";
                responseMessagesList.emplace_back(responseMessage);
                continue;
            }
            else
            {
                // 处理常规指令

                std::shared_ptr<CommandParser> commandParser = flyweightFactory->getParser(command); // 获取解析器

                try
                {
                    responseMessage = commandParser->parse(tokens);
                }
                catch (const std::exception &e)
                {
                    responseMessage = "Error processing command '" + command + "': " + e.what();
                }
                responseMessagesList.emplace_back(responseMessage);
            }
        }
    }
    string res = "";
    for (int i = 0; i < responseMessagesList.size(); i++)
    {
        std::string responseMessage = std::to_string(i + 1) + ")" + responseMessagesList[i];
        res += responseMessage;
        if (i != responseMessagesList.size() - 1)
        {
            res += "\n";
        }
    }

    return res;
}

/**
 * 处理客户端发送的数据，根据数据内容执行相应的操作。
 *
 * @param receivedData 客户端发送的字符串数据。
 * @return 返回处理结果的字符串消息。如果接收到的数据长度大于0，则返回对应的处理结果；否则返回"nil"表示没有数据可读；如果发生错误，则返回"error"。
 */
string RedisServer::handleClient(string receivedData)
{
    // 获取接收到的数据的长度
    size_t bytesRead = receivedData.length();
    // 如果数据长度大于0，则处理数据
    if (bytesRead > 0)
    {
        // 使用istringstream将接收到的字符串分割为多个命令
        std::istringstream iss(receivedData);
        std::string command;
        std::vector<std::string> tokens;
        while (iss >> command)
        { // 以空格分割字符串
            tokens.push_back(command);
        }

        // 如果命令列表不为空
        if (!tokens.empty())
        {
            // 获取第一个命令
            command = tokens.front();
            std::string responseMessage;
            // 如果命令是"quit"或"exit"，则返回"stop"并结束方法
            if (command == "quit" || command == "exit")
            {
                responseMessage = "stop";
                return responseMessage;
            }
            // 如果命令是"multi"，则开始一个新的事务
            else if (command == "multi")
            {
                // 如果已经开始了事务，则返回错误信息
                if (startMulti)
                {
                    responseMessage = "Open the transaction repeatedly!";
                    return responseMessage;
                }
                // 否则，开始新的事务
                startMulti = true;
                // 清空命令队列
                std::queue<std::string> empty;
                std::swap(empty, commandsQueue);
                responseMessage = "OK";
                return responseMessage;
            }
            // 如果命令是"exec"，则执行事务
            else if (command == "exec")
            {
                // 如果没有开始事务，则返回错误信息
                if (startMulti == false)
                {
                    responseMessage = "No transaction is opened!";
                    return responseMessage;
                }
                // 否则，结束事务
                startMulti = false;
                // 如果没有回退，则执行事务
                if (!fallback)
                {
                    responseMessage = executeTransaction(commandsQueue);
                    return responseMessage;
                }
                else
                {
                    // 如果有回退，则丢弃事务并返回错误信息
                    fallback = false;
                    responseMessage = "(error) EXECABORT Transaction discarded because of previous errors.";
                    return responseMessage;
                }
            }
            // 如果命令是"discard"，则丢弃事务
            else if (command == "discard")
            {
                startMulti = false;
                fallback = false;
                responseMessage = "OK";
                return responseMessage;
            }
            // 如果命令是常规指令
            else
            {
                // 如果没有开始事务，则处理常规指令
                if (!startMulti)
                {
                    // 获取对应的命令解析器
                    std::shared_ptr<CommandParser> commandParser = flyweightFactory->getParser(command);
                    // 如果命令解析器不存在，则返回错误信息
                    if (commandParser == nullptr)
                    {
                        responseMessage = "Error: Command '" + command + "' not recognized.";
                    }
                    else
                    {
                        try
                        {
                            // 尝试解析命令并获取响应消息
                            responseMessage = commandParser->parse(tokens);
                        }
                        catch (const std::exception &e)
                        {
                            // 如果解析过程中出现异常，则返回错误信息
                            responseMessage = "Error processing command '" + command + "': " + e.what();
                        }
                    }
                    // 返回响应消息
                    return responseMessage;
                }
                // 如果已经开始事务，则将命令添加到事务队列中
                else
                {
                    // 获取对应的命令解析器
                    std::shared_ptr<CommandParser> commandParser = flyweightFactory->getParser(command);
                    // 如果命令解析器不存在，则设置回退标志并返回错误信息
                    if (commandParser == nullptr)
                    {
                        fallback = true;
                        responseMessage = "Error: Command '" + command + "' not recognized.";
                        return responseMessage;
                    }
                    else
                    {
                        // 将命令添加到队列中并返回"QUEUED"信息
                        commandsQueue.emplace(receivedData);
                        responseMessage = "QUEUED";
                        return responseMessage;
                    }
                }
            }
        }
    }
    // 如果接收到的数据长度为0，则返回"nil"并继续循环
    else
    {
        return "nil";
    }
    // 如果以上条件都不满足，则返回"error"
    return "error";
}

// 替换字符串中的指定字符
/**
 * 在给定的文本中，将指定的旧文本替换为新的文本。
 *
 * @param text 需要进行替换操作的原始文本。
 * @param toReplaceText 需要被替换的旧文本。
 * @param newText 用于替换旧文本的新文本。
 */

void RedisServer::replaceText(std::string &text, const std::string &toReplaceText, const std::string &newText)
{
    size_t start_pos = text.find(toReplaceText);
    while (start_pos != std::string::npos)
    {
        text.replace(start_pos, toReplaceText.length(), newText);
        start_pos = text.find(toReplaceText, start_pos + newText.length());
    }
}
// 获取当前时间
/**
 * 获取当前系统时间，并以"年-月-日 时：分：秒"的格式返回。
 *
 * @return 返回一个字符串，表示当前的日期和时间。
 */
std::string RedisServer::getDate()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm;
    localtime_r(&now_c, &local_tm);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
// 信号处理函数
/**
 * 处理Redis服务器接收到的信号。
 *
 * @param sig 接收到的信号，通常为SIGINT（中断信号）。
 * 如果接收到的是SIGINT信号，那么会调用CommandParser::getRedisHelper()->flush()方法清空当前Redis服务器中的所有数据，然后退出程序。
 */
void RedisServer::signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        CommandParser::getRedisHelper()->flush();
        exit(0);
    }
}

/**
 * 初始化RedisServer对象。
 *
 * @param port 服务器监听的端口号。
 * @param logoFilePath 服务器logo文件的路径。
 */
RedisServer::RedisServer(int port, const std::string &logoFilePath)
    : port(port), logoFilePath(logoFilePath),
    flyweightFactory(new ParserFlyweightFactory())
{
    pid = getpid();
}
