#include "RedisHelper.h"
#include "FileCreator.h"

/**
 * 使用RedisHelper类中的flush方法，将redis数据库中的数据写入到文件中。
 *
 * @param 无参数。
 *
 * @return 无返回值。
 */
void RedisHelper::flush()
{
    // 打开文件并覆盖写入
    std::string filePath = getFilePath();
    std::ofstream outputFile(filePath);
    // 检查文件是否成功打开
    if (!outputFile)
    {
        std::cout << "文件：" << filePath << "打开失败" << std::endl;
        return;
    }
    auto currentNode = redisDataBase->getHead();
    while (currentNode != nullptr)
    {
        std::string key = currentNode->key;
        RedisValue value = currentNode->value;
        if (!key.empty())
            outputFile << key << ":" << value.dump() << std::endl;
        currentNode = currentNode->forward[0];
    }
    // 关闭文件
    outputFile.close();
}

/**
 * 获取Redis数据库文件的完整路径。
 *
 * @return 返回一个字符串，表示Redis数据库文件的完整路径。该路径由默认数据库文件夹名、数据库文件名和数据库索引号拼接而成。
 */
std::string RedisHelper::getFilePath()
{
    std::string folder = DEFAULT_DB_FOLDER;                         // 文件夹名
    std::string fileName = DATABASE_FILE_NAME;                      // 文件名
    std::string filePath = folder + "/" + fileName + dataBaseIndex; // 文件路径
    return filePath;
}

// 从文件中加载
/**
 * 使用给定的路径加载Redis数据库中的数据。
 *
 * @param loadPath 用于加载数据的字符串路径。
 */
void RedisHelper::loadData(std::string loadPath)
{
    redisDataBase->loadFile(loadPath);
}

// 选择数据库
/**
 * 选择指定的Redis数据库。
 *
 * @param index 要选择的数据库的索引。如果索引超出范围，则返回错误信息。
 * @return 如果成功选择了数据库，返回"OK"；否则返回错误信息。
 */
std::string RedisHelper::select(int index)
{
    if (index < 0 || index > DATABASE_FILE_NUMBER - 1)
    {
        return "database index out of range.";
    }
    flush(); // 选择数据库之前先写入一下
    redisDataBase = std::make_shared<SkipList<std::string, RedisValue>>();
    dataBaseIndex = std::to_string(index);
    std::string filePath = getFilePath(); // 根据选择的数据库，修改文件路径，然后加载

    loadData(filePath);
    return "OK";
}
// key操作命令
// 获取所有键
// 语法：keys pattern
// 127.0.0.1:6379> keys *
// 1) "javastack"
// *表示通配符，表示任意字符，会遍历所有键显示所有的键列表，时间复杂度O(n)，在生产环境不建议使用。
/**
 * 使用给定的模式从Redis数据库中获取所有匹配的键。
 *
 * @param pattern 用于匹配键的模式字符串。
 * @return 返回一个包含所有匹配键的字符串，每个键占一行，行号和键值之间用括号分隔。如果数据库为空，则返回"this database is empty!"。
 */
std::string RedisHelper::keys(const std::string pattern)
{
    std::string res = "";
    auto node = redisDataBase->getHead()->forward[0];
    int count = 0;
    while (node != nullptr)
    {
        res += std::to_string(++count) + ") " + "\"" + node->key + "\"" + "\n";
        node = node->forward[0];
    }
    if (!res.empty())
        res.pop_back();
    else
    {
        res = "this database is empty!";
    }
    return res;
}
// 获取键总数
// 语法：dbsize
// 127.0.0.1:6379> dbsize
// (integer) 6
// 获取键总数时不会遍历所有的键，直接获取内部变量，时间复杂度O(1)。
/**
 * 获取Redis数据库的大小。
 *
 * @return 返回一个字符串，该字符串以"(integer) "开头，后面跟着Redis数据库的大小。
 */
std::string RedisHelper::dbsize() const
{
    std::string res = "(integer) " + std::to_string(redisDataBase->size());
    return res;
}
// 查询键是否存在
// 语法：exists key [key ...]
// 127.0.0.1:6379> exists javastack java
// (integer) 2
// 查询查询多个，返回存在的个数。
/**
 * 检查给定的键是否存在于Redis数据库中，并返回存在的键的数量。
 *
 * @param keys 一个包含要检查的键的字符串向量。
 * @return 返回一个字符串，表示在Redis数据库中找到的键的数量。格式为"(integer) " +找到的键的数量。
 */
std::string RedisHelper::exists(const std::vector<std::string> &keys)
{
    int count = 0;
    for (auto &key : keys)
    {
        if (redisDataBase->searchItem(key) != nullptr)
        {
            count++;
        }
    }
    std::string res = "(integer) " + std::to_string(count);
    return res;
}
// 删除键
// 语法：del key [key ...]
// 127.0.0.1:6379> del java javastack
// (integer) 1
// 可以删除多个，返回删除成功的个数。
/**
 * 使用给定的键列表，从Redis数据库中删除相应的项。
 *
 * @param keys 一个包含要删除的键的字符串向量。
 * @return 返回一个字符串，表示成功删除的项的数量。
 */
std::string RedisHelper::del(const std::vector<std::string> &keys)
{
    int count = 0;
    for (auto &key : keys)
    {
        if (redisDataBase->deleteItem(key))
        {
            count++;
        }
    }
    std::string res = "(integer) " + std::to_string(count);
    return res;
}

// 更改键名称
// 语法：rename key newkey
// 127.0.0.1:6379[2]> rename javastack javastack123
// OK
/**
 * 使用给定的新名称重命名Redis数据库中的现有项。
 *
 * @param oldName 要重命名的现有项的当前名称。
 * @param newName 要将现有项重命名为的新名称。
 * @return 如果成功找到并重命名了现有项，则返回"OK"；如果找不到现有的项，则返回一个包含错误消息的字符串。
 */
std::string RedisHelper::rename(const std::string &oldName, const std::string &newName)
{
    auto currentNode = redisDataBase->searchItem(oldName);
    std::string resMessage = "";
    if (currentNode == nullptr)
    {
        resMessage += oldName + " does not exist!";
        return resMessage;
    }
    currentNode->key = newName;
    resMessage = "OK";
    return resMessage;
}

// 字符串操作命令
// 存放键值
// 语法：set key value [EX seconds] [PX milliseconds] [NX|XX]
// nx：如果key不存在则建立，xx：如果key存在则修改其值，也可以直接使用setnx/setex命令。
/**
 * 使用给定的键和值以及设置模式，在Redis数据库中设置一个键值对。
 *
 * @param key 要设置的键。
 * @param value 要设置的值。
 * @param model 设置的模式，可以是XX（如果键不存在则设置）或NX（仅当键不存在时设置）。
 * @return 如果成功设置键值对，返回"OK"；否则，根据具体错误返回相应的错误信息。
 */
std::string RedisHelper::set(const std::string &key, const RedisValue &value, const SET_MODEL model)
{

    if (model == XX)
    {
        return setex(key, value);
    }
    else if (model == NX)
    {
        return setnx(key, value);
    }
    else
    {
        auto currentNode = redisDataBase->searchItem(key);
        if (currentNode == nullptr)
        {
            setnx(key, value);
        }
        else
        {
            setex(key, value);
        }
    }

    return "OK";
}

/**
 * 使用给定的键和值在Redis数据库中设置一个新的项。如果键已经存在，则返回错误信息；否则，添加新的项并返回"OK"。
 *
 * @param key 要设置的键。
 * @param value 要设置的值。
 * @return 如果键已存在，返回"key: "+ key +"  exists!"；否则，返回"OK"。
 */
std::string RedisHelper::setnx(const std::string &key, const RedisValue &value)
{
    auto currentNode = redisDataBase->searchItem(key);
    if (currentNode != nullptr)
    {
        return "key: " + key + "  exists!";
    }
    else
    {
        redisDataBase->addItem(key, value);
    }
    return "OK";
}
/**
 * 使用给定的键和值设置Redis数据库中的项。如果键不存在，则返回错误消息；否则，更新该键的值并返回"OK"。
 *
 * @param key   需要设置的键。
 * @param value 需要设置的值。
 * @return 如果键存在，则返回"OK"；如果键不存在，则返回错误消息。
 */
std::string RedisHelper::setex(const std::string &key, const RedisValue &value)
{
    auto currentNode = redisDataBase->searchItem(key);
    if (currentNode == nullptr)
    {
        return "key: " + key + " does not exist!";
    }
    else
    {
        currentNode->value = value;
    }
    return "OK";
}
// 127.0.0.1:6379> set javastack 666
// OK
// 获取键值
// 语法：get key
// 127.0.0.1:6379[2]> get javastack
// "666"
/**
 * 使用给定的键从Redis数据库中获取对应的值。
 *
 * @param key 需要查询的键。
 * @return 如果键存在，返回对应的值；如果键不存在，返回错误信息。
 */
std::string RedisHelper::get(const std::string &key)
{
    auto currentNode = redisDataBase->searchItem(key);
    if (currentNode == nullptr)
    {
        return "key: " + key + " does not exist!";
    }
    return currentNode->value.dump();
}
// 值递增/递减
// 如果字符串中的值是数字类型的，可以使用incr命令每次递增，不是数字类型则报错。

// 语法：incr key
// 127.0.0.1:6379[2]> incr javastack
// (integer) 667
// 一次想递增N用incrby命令，如果是浮点型数据可以用incrbyfloat命令递增。
/**
 * 使用Redis的incrby命令，将给定键的值增加1。
 *
 * @param key 需要增加值的键名。
 * @return 返回增加后的值。
 */
std::string RedisHelper::incr(const std::string &key)
{
    return incrby(key, 1);
}
/**
 * 使用给定的增量值增加Redis数据库中指定键的值。如果键不存在，则创建一个新的键并设置其值为增量值。
 * 如果键存在但其值不是数字类型，则返回错误信息。
 *
 * @param key 需要增加值的Redis键。
 * @param increment 需要增加的值。
 * @return 如果操作成功，返回"(integer) " + value，其中value是新的键值；如果键不存在或其值不是数字类型，返回相应的错误信息。
 */
std::string RedisHelper::incrby(const std::string &key, int increment)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string value = "";
    if (currentNode == nullptr)
    {
        value = std::to_string(increment);
        redisDataBase->addItem(key, value);
        return "(integer) " + value;
    }
    value = currentNode->value.dump();
    // 去掉双引号
    value.erase(0, 1);
    value.erase(value.size() - 1);
    for (char ch : value)
    {
        if (!isdigit(ch))
        {
            std::string res = "The value of " + key + " is not a numeric type";
            return res;
        }
    }
    int curValue = std::stoi(value) + increment;
    value = std::to_string(curValue);
    currentNode->value = value;
    std::string res = "(integer) " + value;
    return res;
}
/**
 * 使用给定的增量值增加指定键的值。如果键不存在，则创建一个新的键并设置其值为增量值。
 * 如果键存在但值不是数字类型，则返回错误信息。
 *
 * @param key 需要增加值的键。
 * @param increment 需要增加的值。
 * @return 如果操作成功，返回"(float) " + value；如果键不存在，返回"(float) " + increment；如果键存在但值不是数字类型，返回"The value of " + key + " is not a numeric type"。
 */
std::string RedisHelper::incrbyfloat(const std::string &key, double increment)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string value = "";
    if (currentNode == nullptr)
    {
        value = std::to_string(increment);
        redisDataBase->addItem(key, value);
        return "(float) " + value;
    }
    value = currentNode->value.dump();
    value.erase(0, 1);
    value.erase(value.size() - 1);
    double curValue = 0.0;
    try
    {
        curValue = std::stod(value) + increment;
    }
    catch (std::invalid_argument const &e)
    {
        return "The value of " + key + " is not a numeric type";
    }
    value = std::to_string(curValue);
    currentNode->value = value;
    std::string res = "(float) " + value;
    return res;
}
// 同样，递减使用decr、decrby命令。
/**
 * 使用给定的键值，对Redis中的数值进行减一操作。
 *
 * @param key 需要进行减一操作的键值。
 * @return 返回减一操作后的结果。
 */
std::string RedisHelper::decr(const std::string &key)
{
    return incrby(key, -1);
}
/**
 * 使用给定的增量减少Redis中指定键的值。
 *
 * @param key 需要修改的Redis键。
 * @param increment 要减去的增量值。
 * @return 返回操作后的键值。
 */
std::string RedisHelper::decrby(const std::string &key, int increment)
{
    return incrby(key, -increment);
}
// 批量存放键值
// 语法：mset key value [key value ...]
// 127.0.0.1:6379[2]> mset java1 1 java2 2 java3 3
// OK

/**
 * 使用给定的键值对列表，通过RedisHelper类的mset方法批量设置键值对。
 *
 * @param items 一个包含键值对的字符串向量，其中偶数索引处的元素为键，奇数索引处的元素为对应的值。
 * @return 如果参数数量正确且所有键值对都成功设置，则返回"OK"；如果参数数量不正确，则返回错误信息"wrong number of arguments for MSET."。
 */
std::string RedisHelper::mset(std::vector<std::string> &items)
{
    if (items.size() % 2 != 0)
    {
        return "wrong number of arguments for MSET.";
    }
    for (int i = 0; i < items.size(); i += 2)
    {
        std::string key = items[i];
        std::string value = items[i + 1];
        set(key, value);
    }
    return "OK";
}
// 获取获取键值
// 语法：mget key [key ...]
// 127.0.0.1:6379[2]> mget java1 java2
// 1) "1"
// 2) "2"
// Redis接收的是UTF-8的编码，如果是中文一个汉字将占3位返回。
/**
 * 使用给定的键列表，从Redis数据库中获取对应的值。
 *
 * @param keys 一个包含要查询的键的字符串向量。
 * @return 如果所有键都存在，返回一个字符串，其中包含每个键及其对应值的列表；如果某个键不存在，则在对应的位置上显示"(nil)"；如果传入的键列表为空，则返回错误信息"wrong number of arguments for MGET."。
 */
std::string RedisHelper::mget(std::vector<std::string> &keys)
{
    if (keys.size() == 0)
    {
        return "wrong number of arguments for MGET.";
    }
    std::vector<std::string> values;
    std::string res = "";
    for (int i = 0; i < keys.size(); i++)
    {
        std::string &key = keys[i];
        std::string value = "";
        auto currentNode = redisDataBase->searchItem(key);
        if (currentNode == nullptr)
        {
            value = "(nil)";
            res += std::to_string(i + 1) + ") " + value + "\n";
        }
        else
        {
            value = currentNode->value.dump();
            res += std::to_string(i + 1) + ") " + value + "\n";
        }
    }
    res.pop_back();
    return res;
}
// 获取值长度
// 语法：strlen key
// 127.0.0.1:6379[2]> strlen javastack (integer) 3
/**
 * 获取Redis数据库中指定键值的长度。
 *
 * @param key 需要查询长度的键值。
 * @return 如果键值存在，返回键值的长度；如果键值不存在，返回"(integer) 0"。
 */
std::string RedisHelper::strlen(const std::string &key)
{
    auto currentNode = redisDataBase->searchItem(key);
    if (currentNode == nullptr)
    {
        return "(integer) 0";
    }
    return "(integer) " + std::to_string(currentNode->value.dump().size());
}
// 追加内容
// 语法：append key value
// 127.0.0.1:6379[2]> append javastack hi
// (integer) 5
// 向键值尾部添加，如上命令执行后由666变成666hi
/**
 * 使用给定的键和值在Redis数据库中添加或追加数据。
 *
 * @param key   要添加或追加数据的键。
 * @param value 要添加或追加的数据的值。
 * @return 如果键不存在，则添加新的键值对并返回新添加的值的大小；如果键已存在，则追加新的值到现有值并返回追加后的值的大小。
 */
std::string RedisHelper::append(const std::string &key, const std::string &value)
{
    auto currentNode = redisDataBase->searchItem(key);
    if (currentNode == nullptr)
    {
        redisDataBase->addItem(key, value);
        return "(integer) " + std::to_string(value.size());
    }
    currentNode->value = currentNode->value.dump() + value;
    return "(integer) " + std::to_string(currentNode->value.dump().size());
}

/**
 * 构造函数，用于初始化RedisHelper对象。
 * 在默认数据库文件夹中创建文件和数据库文件，然后加载数据。
 *
 * @param 无参数
 * @return 无返回值
 */
RedisHelper::RedisHelper()
{
    FileCreator::createFolderAndFiles(DEFAULT_DB_FOLDER, DATABASE_FILE_NAME, DATABASE_FILE_NUMBER);
    std::string filePath = getFilePath();
    loadData(filePath);
}
RedisHelper::~RedisHelper() { flush(); }

// 列表操作
//  LPUSH key value：将一个值插入到列表头部。
//  RPUSH key value：将一个值插入到列表尾部。
//  LPOP key：移出并获取列表的第一个元素。
//  RPOP key：移出并获取列表的最后一个元素。
//  LRANGE key start stop：获取列表指定范围内的元素。
/**
 * 使用给定的键和值，将值推入Redis数据库中对应的列表。如果键不存在，则创建一个新的列表并添加值；如果键存在但值不是列表，则返回错误信息；如果键存在且值是列表，则在列表头部插入值。
 *
 * @param key 要操作的Redis数据库中的键。
 * @param value 要插入的值。
 * @return 如果操作成功，返回一个字符串，表示列表的新长度；如果键已存在但值不是列表，返回错误信息。
 */
std::string RedisHelper::lpush(const std::string &key, const std::string &value)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int size = 0;
    if (currentNode == nullptr)
    {
        std::vector<RedisValue> data;
        RedisValue redisList(data);
        RedisValue::array &valueList = redisList.arrayItems();
        valueList.insert(valueList.begin(), value);
        redisDataBase->addItem(key, redisList);
        size = 1;
    }
    else
    {
        if (currentNode->value.type() != RedisValue::ARRAY)
        {
            resMessage = "The key:" + key + " " + "already exists and the value is not a list!";
            return resMessage;
        }
        else
        {
            RedisValue::array &valueList = currentNode->value.arrayItems();
            valueList.insert(valueList.begin(), value);
            size = valueList.size();
        }
    }

    resMessage = "(integer) " + std::to_string(size);
    return resMessage;
}
/**
 * 使用给定的键和值，将值添加到Redis数据库中的列表中。如果键不存在，则创建一个新的列表并添加值。如果键存在但对应的值不是列表，则返回错误消息。
 *
 * @param key 要添加到Redis数据库的键。
 * @param value 要添加到Redis数据库的值。
 * @return 如果操作成功，返回一个字符串，表示列表的新大小；如果键存在但对应的值不是列表，返回一个错误消息。
 */
std::string RedisHelper::rpush(const std::string &key, const std::string &value)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int size = 0;
    if (currentNode == nullptr)
    {
        std::vector<RedisValue> data;
        RedisValue redisList(data);
        RedisValue::array &valueList = redisList.arrayItems();
        valueList.push_back(value);
        redisDataBase->addItem(key, redisList);
        size = 1;
    }
    else
    {
        if (currentNode->value.type() != RedisValue::ARRAY)
        {
            resMessage = "The key:" + key + " " + "already exists and the value is not a list!";
            return resMessage;
        }
        else
        {
            RedisValue::array &valueList = currentNode->value.arrayItems();
            valueList.push_back(value);
            size = valueList.size();
        }
    }

    resMessage = "(integer) " + std::to_string(size);
    return resMessage;
}
/**
 * 使用给定的键从Redis数据库中获取并移除列表的第一个元素。
 *
 * @param key 要查询的键名。
 * @return 如果键存在并且对应的值是一个数组，返回数组的第一个元素的字符串表示；如果键不存在或者对应的值不是一个数组，返回"(nil)"。
 */
std::string RedisHelper::lpop(const std::string &key)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int size = 0;
    if (currentNode == nullptr || currentNode->value.type() != RedisValue::ARRAY)
    {
        resMessage = "(nil)";
    }
    else
    {

        RedisValue::array &valueList = currentNode->value.arrayItems();
        resMessage = (*valueList.begin()).dump();
        valueList.erase(valueList.begin());
        resMessage.erase(0, 1);
        resMessage.erase(resMessage.size() - 1);
    }
    return resMessage;
}
/**
 * 使用给定的键从Redis数据库中移除并返回最后一个元素。
 *
 * @param key 要操作的Redis键。
 * @return 如果键存在并且其值是一个数组，则返回该数组的最后一个元素；如果键不存在或其值不是一个数组，则返回"(nil)"。
 */
std::string RedisHelper::rpop(const std::string &key)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int size = 0;
    if (currentNode == nullptr || currentNode->value.type() != RedisValue::ARRAY)
    {
        resMessage = "(nil)";
    }
    else
    {
        RedisValue::array &valueList = currentNode->value.arrayItems();
        resMessage = (valueList.back()).dump();
        valueList.pop_back();
        resMessage.erase(0, 1);
        resMessage.erase(resMessage.size() - 1);
    }
    return resMessage;
}
/**
 * 使用给定的键、起始和结束索引，从Redis数据库中获取一个数组类型的值，并以字符串形式返回。
 *
 * @param key 要查询的Redis键。
 * @param start 要获取的数组元素的起始索引（包含）。
 * @param end 要获取的数组元素的结束索引（包含）。
 * @return 如果找到了对应的键并且其值是一个数组，则返回该数组中指定范围内的元素组成的字符串；如果找不到对应的键或者其值不是一个数组，则返回"(nil)"或"(empty list or set)"。
 */
std::string RedisHelper::lrange(const std::string &key, const std::string &start, const std::string &end)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int size = 0;
    if (currentNode == nullptr || currentNode->value.type() != RedisValue::ARRAY)
    {
        resMessage = "(nil)";
    }
    else
    {

        RedisValue::array &valueList = currentNode->value.arrayItems();
        int left = std::stoi(start);
        int right = std::stoi(end);
        left = std::max(left, 0);
        right = std::min(right, int(valueList.size() - 1));
        if (right < left || left >= valueList.size())
        {
            resMessage = "(empty list or set)";
        }
        for (int i = left; i <= right; i++)
        {
            auto item = valueList[i];
            resMessage += std::to_string(i + 1) + ") " + item.dump();
            if (i != right)
            {
                resMessage += "\n";
            }
        }
    }
    return resMessage;
}

// 哈希表操作
// HSET key field value：向哈希表中添加一个字段及其值。
// HGET key field：获取哈希表中指定字段的值。
// HDEL key field：删除哈希表 key 中的一个或多个指定字段。
// HKEYS key：获取哈希表中的所有字段名。
// HVALS key：获取哈希表中的所有值。

/**
 * 使用给定的键和字段值，将数据添加到Redis数据库中。如果键不存在，则创建一个新的哈希表并添加数据。如果键存在并且其值不是哈希表，则返回错误消息。如果键存在并且其值是哈希表，则更新哈希表并返回成功插入的字段数。
 *
 * @param key 要添加到Redis数据库中的键。
 * @param filed 一个包含字段名和字段值的向量。每个字段名后面紧跟着一个字段值。
 * @return 如果操作成功，返回一个字符串，表示成功插入的字段数；如果键已存在但其值不是哈希表，返回一个错误消息。
 */
std::string RedisHelper::hset(const std::string &key, const std::vector<std::string> &filed)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int count = 0;
    if (currentNode == nullptr)
    {
        std::map<std::string, RedisValue> data;
        RedisValue redisHash(data);
        RedisValue::object &valueMap = redisHash.objectItems();
        for (int i = 0; i < filed.size(); i += 2)
        {
            std::string hkey = filed[i];
            RedisValue hval = filed[i + 1];
            if (!valueMap.count(hkey))
            {
                valueMap[hkey] = hval;
                count++;
            }
        }
        redisDataBase->addItem(key, valueMap);
    }
    else
    {
        if (currentNode->value.type() != RedisValue::OBJECT)
        {
            resMessage = "The key:" + key + " " + "already exists and the value is not a hashtable!";
            return resMessage;
        }
        else
        {
            RedisValue::object &valueMap = currentNode->value.objectItems();
            for (int i = 0; i < filed.size(); i += 2)
            {
                std::string hkey = filed[i];
                RedisValue hval = filed[i + 1];
                if (!valueMap.count(hkey))
                {
                    valueMap[hkey] = hval;
                    count++;
                }
            }
        }
    }

    resMessage = "(integer) " + std::to_string(count);
    return resMessage;
}
/**
 * 使用给定的键和字段从Redis数据库中获取值。
 *
 * @param key 要搜索的Redis数据库中的键。
 * @param filed 要获取的值在Redis对象中的字段。
 * @return 如果找到对应的键和字段，返回该字段的值；否则返回"(nil)"。
 */
std::string RedisHelper::hget(const std::string &key, const std::string &filed)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int count = 0;
    if (currentNode == nullptr || currentNode->value.type() != RedisValue::OBJECT)
    {
        resMessage = "(nil)";
    }
    else
    {
        RedisValue::object &valueMap = currentNode->value.objectItems();
        if (!valueMap.count(filed))
        {
            resMessage = "(nil)";
        }
        else
        {
            resMessage = valueMap[filed].stringValue();
        }
    }
    return resMessage;
}
/**
 * 使用给定的键和字段列表，从Redis数据库中删除对应的哈希表项。
 *
 * @param key 要操作的Redis哈希表的键。
 * @param filed 包含要从哈希表中删除的字段的列表。
 * @return 返回一个字符串，表示成功删除的字段数量。
 */
std::string RedisHelper::hdel(const std::string &key, const std::vector<std::string> &filed)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int count = 0;
    if (currentNode == nullptr || currentNode->value.type() != RedisValue::OBJECT)
    {
        count = 0;
    }
    else
    {
        RedisValue::object &valueMap = currentNode->value.objectItems();
        for (auto &hkey : filed)
        {
            if (valueMap.count(hkey))
            {
                count++;
                valueMap.erase(hkey);
            }
        }
    }
    resMessage = "(integer) " + std::to_string(count);
    return resMessage;
}

/**
 * 使用给定的键，从Redis数据库中获取对应的哈希表的所有键。
 *
 * @param key 要查询的键名。
 * @return 如果键不存在，返回"The key:[key] does not exist!"；如果键存在但值不是哈希表，返回"The key:[key] already exists and the value is not a hashtable!"；如果键存在且值为哈希表，则返回所有键的列表，每个键后面跟一个换行符。
 */
std::string RedisHelper::hkeys(const std::string &key)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int count = 0;
    if (currentNode == nullptr)
    {
        resMessage = "The key:" + key + " " + "does not exist!";
        return resMessage;
    }
    else
    {
        if (currentNode->value.type() != RedisValue::OBJECT)
        {
            resMessage = "The key:" + key + " " + "already exists and the value is not a hashtable!";
            return resMessage;
        }
        else
        {
            RedisValue::object &valueMap = currentNode->value.objectItems();
            int index = 1;
            for (auto hkey : valueMap)
            {
                resMessage += std::to_string(index) + ") " + hkey.first + "\n";
                index++;
            }
            resMessage.erase(resMessage.size() - 1);
        }
    }
    return resMessage;
}

/**
 * 使用给定的键从Redis数据库中获取对应的哈希表的所有值。
 *
 * @param key 要查询的键，该键对应的值应为哈希表。
 * @return 如果键不存在，返回"The key:[key] does not exist!"；如果键存在但对应的值不是哈希表，返回"The key:[key] already exists and the value is not a hashtable!"；如果键存在且对应的值为哈希表，则返回所有哈希表的值，每个值之间用换行符分隔。
 */
std::string RedisHelper::hvals(const std::string &key)
{
    auto currentNode = redisDataBase->searchItem(key);
    std::string resMessage = "";
    int count = 0;
    if (currentNode == nullptr)
    {
        resMessage = "The key:" + key + " " + "does not exist!";
        return resMessage;
    }
    else
    {
        if (currentNode->value.type() != RedisValue::OBJECT)
        {
            resMessage = "The key:" + key + " " + "already exists and the value is not a hashtable!";
            return resMessage;
        }
        else
        {
            RedisValue::object &valueMap = currentNode->value.objectItems();
            int index = 1;
            for (auto hkey : valueMap)
            {
                resMessage += std::to_string(index) + ") " + hkey.second.stringValue() + "\n";
                index++;
            }
            resMessage.erase(resMessage.size() - 1);
        }
    }
    return resMessage;
}