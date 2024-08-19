#ifndef DUMP_H
#define DUMP_H
#include<cmath>
#include<string>
#include"RedisValue.h"

// 表示null值
struct NullStruct{
    bool operator==(NullStruct) const{ return true; }
    bool operator<(NullStruct) const { return false; }
};

// 用于输出null值到字符串中
/**
 * 将"null"字符串添加到给定的输出字符串中。
 *
 * @param NullStruct 一个空的结构体，此参数在函数中并未被使用。
 * @param out 用于存储添加了"null"字符串的输出字符串。
 */
static void dump(NullStruct, std::string &out) {
    out += "null";
}

// 用于将浮点数值转换为字符串并追加到输出字符串中
/**
 * 将给定的double值转换为字符串，并添加到提供的输出字符串中。如果值为有限数，则将其格式化为17位小数的字符串；否则，添加"null"到输出字符串中。
 *
 * @param value 需要转换的double值。
 * @param out   用于存储转换结果的字符串。
 */
static void dump(double value, std::string &out) {
    if (std::isfinite(value)) { // 检查值是否为有限数
        char buf[32];
        snprintf(buf, sizeof buf, "%.17g", value); // 格式化浮点数
        out += buf;
    } else {
        out += "null"; // 对于非有限数，输出为null
    }
}

// 用于将整数值转换为字符串并追加到输出字符串中
/**
 * 将整数值转换为字符串，并添加到给定的输出字符串中。
 *
 * @param value 需要转换的整数值。
 * @param out   用于存储转换后的字符串的引用。
 */
static void dump(int value, std::string &out) {
    char buf[32];
    snprintf(buf, sizeof buf, "%d", value); // 格式化整数
    out += buf;
}

// 用于将布尔值转换为字符串并追加到输出字符串中
/**
 * 将布尔值转换为字符串形式，并添加到给定的输出字符串中。
 *
 * @param value 需要转换的布尔值。
 * @param out   用于存储转换结果的字符串。
 */
static void dump(bool value, std::string &out) {
    out += value ? "true" : "false";
}

// 用于将字符串值进行转义处理并追加到输出字符串中
static void dump(const std::string &value, std::string &out) {
    out += '"';
    for (size_t i = 0; i < value.length(); i++) {
        const char ch = value[i];
        // 根据字符进行相应的转义处理
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<uint8_t>(ch) <= 0x1f) {
                    char buf[8];
                    snprintf(buf, sizeof buf, "\\u%04x", ch); // 对控制字符进行Unicode转义
                    out += buf;
                } else {
                    out += ch;
                }
        }
    }
    out += '"';
}

// 用于将Json数组转换为字符串并追加到输出字符串中
static void dump(const RedisValue::array &values, std::string &out) {
    bool first = true;
    out += "[";
    for (const auto &value : values) {
        if (!first) out += ", ";
        value.dump(out);
        first = false;
    }
    out += "]";
}

// 用于将Json对象转换为字符串并追加到输出字符串中
static void dump(const RedisValue::object &values, std::string &out) {
    bool first = true;
    out += "{";
    for (const auto &kv : values) {
        if (!first) out += ", ";
        dump(kv.first, out); // 键名进行转义处理
        out += ": ";
        kv.second.dump(out); // 键值递归调用dump
        first = false;
    }
    out += "}";
}

#endif
