#ifndef GLOBAL_H
#define GLOBAL_H
#include "RedisValueType.h"

// 定义最大深度常量，用于限制JSON解析或序列化的最大深度，防止栈溢出等问题。
static const int max_depth = 200;

// Statics结构体，用于存储JSON值的一些静态实例，如null、true、false等，以及空的字符串、向量和映射。
// 这样做是为了避免重复创建这些常用对象，提高效率。
struct Statics{
    std::shared_ptr<RedisValueType> null = std::make_shared<RedisValueNull>();

    // 定义一个静态的空字符串
    std::string emptyString;

    // 定义一个静态的空Json数组
    std::vector<RedisValue> emptyVector;

    // 定义一个静态的空Json对象映射
    std::map<std::string,RedisValue> emptyMap;

    // 默认构造函数
    Statics(){}
};

// 返回一个静态的Statics实例的引用，保证整个程序中只有一个Statics实例。
static Statics& statics(){
    static Statics s{};
    return s;
}

// 返回一个静态的null Json实例的引用，用于表示JSON中的null值。
static RedisValue & staticNull(){
    static RedisValue redisValueNull;
    return redisValueNull;
}

// 对字符进行转义，用于JSON字符串的序列化。
// 如果字符是可打印的ASCII字符，则返回字符本身和其ASCII码；否则只返回ASCII码。
// 定义一个静态内联函数，输入参数为一个字符，返回值为一个字符串
static inline std::string esc(char c) {
    // 定义一个字符数组buf，长度为12
    char buf[12];
    // 判断输入的字符c是否在0x20到0x7f之间（即ASCII码表中可打印字符的范围）
    if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
        // 如果满足条件，将字符c以单引号包围的形式和对应的ASCII码值格式化到buf中
        snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
    } else {
        // 如果不满足条件，直接将字符c的ASCII码值格式化到buf中
        snprintf(buf, sizeof buf, "(%d)", c);
    }
    // 将buf转换为std::string类型并返回
    return std::string(buf);
}

// 检查一个长整型数值是否在指定的范围内。
static inline bool in_range(long x, long lower, long upper) {
    return (x >= lower && x <= upper);
}

#endif
