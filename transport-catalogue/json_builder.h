#pragma once

#include <string>
#include <vector>

#include "json.h"

namespace json {

class ItemContext;
class KeyItemContext;
class DictItemContext;
class ArrayItemContext;

class Builder {
public:
    Builder() = default;
    KeyItemContext Key(std::string key);
    Builder& Value(Node value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Node Build();
private:
    Node root_ = nullptr;
    std::vector<Node*> nodes_stack_;
    bool is_key_ = false;
    bool is_empty_ = true;
};

class ItemContext {
public:
    ItemContext(Builder& builder);
protected:
    KeyItemContext Key(std::string key);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Builder& builder_;
};

class KeyItemContext : public ItemContext {
public:
    using ItemContext::ItemContext;
    using ItemContext::StartDict;
    using ItemContext::StartArray;
    DictItemContext Value(Node value);
};

class DictItemContext : public ItemContext {
public:
    using ItemContext::ItemContext;
    using ItemContext::Key;
    using ItemContext::EndDict;
};

class ArrayItemContext : public ItemContext {
public:
    using ItemContext::ItemContext;
    using ItemContext::StartDict;
    using ItemContext::StartArray;
    using ItemContext::EndArray;
    ArrayItemContext Value(Node value);
};

} // json