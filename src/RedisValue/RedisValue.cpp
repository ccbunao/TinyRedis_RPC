
#include"Global.h"
#include "Parse.h"


/*************构造函数******************/

RedisValue::RedisValue() noexcept : redisValue(statics().null) {}

RedisValue::RedisValue(std::nullptr_t) noexcept : redisValue(statics().null) {}

RedisValue::RedisValue(const std::string& value) : redisValue(std::make_shared<RedisString>(value)) {}

RedisValue::RedisValue(std::string&& value) : redisValue(std::make_shared<RedisString>(std::move(value))) {}

RedisValue::RedisValue(const char* value) : redisValue(std::make_shared<RedisString>(value)) {}

RedisValue::RedisValue(const RedisValue::array& value) : redisValue(std::make_shared<RedisList>(value)) {}

RedisValue::RedisValue(RedisValue::array&& value) : redisValue(std::make_shared<RedisList>(std::move(value))) {}

RedisValue::RedisValue(const RedisValue::object& value) : redisValue(std::make_shared<RedisObject>(value)) {}

RedisValue::RedisValue(RedisValue::object &&value) : redisValue(std::make_shared<RedisObject>(std::move(value))) {}

/************* Member Functions ******************/

RedisValue::Type RedisValue::type() const {
    return redisValue->type();
}

std::string & RedisValue::stringValue() {
    return redisValue->stringValue();
}

std::vector<RedisValue> & RedisValue::arrayItems() {
    return redisValue->arrayItems();
}

std::map<std::string, RedisValue> & RedisValue::objectItems()  {
    return redisValue->objectItems();
}

/**
 * 使用索引访问RedisValue对象中的元素。
 *
 * @param i 要访问的元素的索引。
 * @return 返回索引对应的RedisValue对象。
 */
RedisValue & RedisValue::operator[] (size_t i)  {
    return (*redisValue)[i];
}

RedisValue & RedisValue::operator[] (const std::string& key) {
    return (*redisValue)[key];
}

/**
 * 获取Redis值类型的字符串表示。
 *
 * @return 返回Redis值类型的字符串表示，如果类型为空，则返回一个空字符串。
 */
std::string& RedisValueType::stringValue() {
    return statics().emptyString;
}

/**
 * 获取RedisValueType的数组项。
 *
 * @return 返回一个包含所有数组项的RedisValue类型的向量。如果该类型没有数组项，则返回一个空向量。
 */
std::vector<RedisValue> & RedisValueType::arrayItems() {
    return statics().emptyVector;
}

/**
 * 获取RedisValueType对象的items。
 *
 * @return 返回一个map，其键为string类型，值为RedisValue类型。如果对象为空，则返回一个空的map。
 */
std::map<std::string, RedisValue> & RedisValueType::objectItems() {
    return statics().emptyMap;
}

/**
 * 重载[]运算符，返回RedisValue类型的静态空值。
 *
 * @param size_t 索引值，但此函数并未使用该参数。
 * @return 返回RedisValue类型的静态空值。
 */
RedisValue& RedisValueType::operator[] (size_t) {
    return staticNull();
}

/**
 * 重载[]运算符，返回RedisValue类型的静态空值。
 *
 * @param key 一个字符串参数，表示键名。
 * @return 返回RedisValue类型的静态空值。
 */
RedisValue& RedisValueType::operator[] (const std::string&) {
    return staticNull();
}

/**
 * 使用给定的键从Redis对象中获取对应的值。
 *
 * @param key 要查找的键，类型为std::string。
 * @return 如果找到对应的键，返回该键对应的值；否则返回一个空的RedisValue对象。
 */
RedisValue& RedisObject::operator[] (const std::string&key) {
    auto it = value.find(key);
    return (it==value.end()) ? staticNull() : it->second;
}

/**
 * 使用索引访问RedisList中的值。
 *
 * @param i 要访问的元素的索引。如果索引大于或等于列表的大小，则返回一个空的RedisValue。
 * @return 如果索引有效，则返回对应索引位置的RedisValue；否则返回一个空的RedisValue。
 */
RedisValue& RedisList::operator[](size_t i ) {
    if(i>=value.size()) return staticNull();
    return value[i];
}

/*比较*/

/**
 * 判断当前RedisValue对象是否等于另一个RedisValue对象。
 *
 * @param other 需要与当前对象进行比较的RedisValue对象。
 * @return 如果两个RedisValue对象的值相等，则返回true；如果两个RedisValue对象的类型不同，则返回false；否则，调用redisValue的equals方法进行比较，并返回结果。
 */
bool RedisValue::operator== (const RedisValue&other) const{
    if (redisValue == other.redisValue)
        return true;
    if (redisValue->type() != other.redisValue->type())
        return false;
    return redisValue->equals(other.redisValue.get());
}

/**
 * 判断当前RedisValue对象是否小于传入的RedisValue对象。
 *
 * @param other 需要与当前对象进行比较的RedisValue对象。
 * @return 如果当前对象小于传入的对象，则返回true；否则返回false。
 */
bool RedisValue::operator< (const RedisValue& other) const{
    if (redisValue == other.redisValue)
        return false;
    if (redisValue->type() != other.redisValue->type())
        return redisValue->type() < other.redisValue->type();
    return redisValue->less(other.redisValue.get());
}


// 定义Json类的成员函数dump，用于将Json对象转化为JSON字符串并追加到out中
void RedisValue::dump(std::string &out) const {
    redisValue->dump(out); // 调用JsonImpl类的dump函数将Json对象转化为JSON字符串并追加到out中
}

// 将字符串转化为Json对象
RedisValue RedisValue::parse(const std::string &in, std::string &err) {
    // 初始化一个Json解析器
    RedisValueParser parser { in, 0, err, false};
    // 解析输入字符串以得到Json结果
    RedisValue result = parser.parseRedisValue(0);

    // 检查是否有尾随的垃圾字符
    parser.consumeGarbage();
    if (parser.failed)
        return RedisValue(); // 如果解析失败，返回一个空的Json对象
    if (parser.i != in.size())
        return parser.fail("unexpected trailing " + esc(in[parser.i])); // 如果输入字符串尚有未解析内容，报告错误

    return result; // 返回解析得到的Json对象
}

/**
 * 解析给定的输入字符串，如果输入为null，则返回错误信息。
 *
 * @param in 需要被解析的输入字符串。
 * @param err 用于存储错误信息的字符串引用。
 * @return 如果输入有效，返回解析后的RedisValue对象；否则返回nullptr并设置错误信息。
 */
RedisValue RedisValue::parse(const char* in, std::string& err){
    if (in) {
            return parse(std::string(in), err);
    } else {
        err = "null input";
        return nullptr;
    }
}
// 解析输入字符串中的多个Json对象
/**
 * 解析输入的字符串，将其转换为RedisValue对象的集合。
 *
 * @param in 需要被解析的字符串。
 * @param parser_stop_pos 解析停止的位置，解析结束后会被更新为当前解析到的位置。
 * @param err 错误信息，如果解析过程中出现错误，该参数会被填充上相应的错误信息。
 * @return 返回一个包含所有解析得到的RedisValue对象的vector。
 */
std::vector<RedisValue> RedisValue::parseMulti(const std::string &in,
                               std::string::size_type &parser_stop_pos,
                               std::string &err) {
    // 初始化一个Json解析器
    RedisValueParser parser { in, 0, err, false };
    parser_stop_pos = 0;
    std::vector<RedisValue> jsonList; // 存储解析得到的多个Json对象的容器

    // 当输入字符串还有内容并且解析未出错时继续
    while (parser.i != in.size() && !parser.failed) {
        jsonList.push_back(parser.parseRedisValue(0)); // 解析Json对象并添加到容器中
        if (parser.failed)
            break; // 如果解析失败，中断循环

        // 检查是否还有其他对象
        parser.consumeGarbage();
        if (parser.failed)
            break; // 如果发现垃圾字符或有错误，中断循环
        parser_stop_pos = parser.i; // 更新停止位置
    }
    return jsonList; // 返回解析得到的Json对象容器
}

/**
 * 解析给定的字符串，返回一个RedisValue类型的向量。
 *
 * @param in 需要被解析的字符串。
 * @param err 如果解析过程中出现错误，将错误信息存储在这个字符串中。
 * @return 返回一个包含解析结果的RedisValue类型的向量。
 */
std::vector<RedisValue> RedisValue::parseMulti(
        const std::string & in,
        std::string & err
    )
{
        std::string::size_type parser_stop_pos;
        return parseMulti(in, parser_stop_pos, err);
}

// 检查 JSON 对象是否具有指定的形状
/**
 * 检查RedisValue对象是否具有指定的形状。
 *
 * @param types 需要匹配的形状，是一个键值对的集合，其中键是JSON对象的成员名，值是对应的类型。
 * @param err   如果函数返回false，这个字符串将被设置为错误信息。
 * @return 如果RedisValue对象具有指定的形状，则返回true，否则返回false，并将错误信息设置到err参数中。
 */
bool RedisValue::hasShape(const shape & types, std::string & err)  {
    // 如果 JSON 不是对象类型，则返回错误
    if (!isObject()) {
        err = "expected JSON object, got " + dump();
        return false;
    }

    // 获取 JSON 对象的所有成员项
    auto obj_items = objectItems();
    
    // 遍历指定的形状
    for (auto & item : types) {
        // 查找 JSON 对象中是否存在形状中指定的项
        const auto it = obj_items.find(item.first);
        
        // 如果找不到项或者项的类型不符合指定的类型，则返回错误
        if (it == obj_items.cend() || it->second.type() != item.second) {
            err = "bad type for " + item.first + " in " + dump();
            return false;
        }
    }

    // 如果所有形状都匹配，则返回 true
    return true;
}