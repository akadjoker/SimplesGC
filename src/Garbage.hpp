#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <deque>

const int GC_THRESHOLD = 1024 * 24;

enum ObjectType
{
    NIL,
    INT,
    REAL,
    STRING,
    POINTER,
    LIST,
    MAP,
    SCOPE,
};

struct Pointer;

typedef void (*OnDeleteFunction)(Pointer *);

class Arena
{
public:
    static Arena &as()
    {
        static Arena arena;
        return arena;
    }

    size_t size() { return _size; }

    void *allocate(size_t size);
    void free(void *p, size_t size);

private:
    Arena()
    {

        blockSize = 1024 * 1024;
        currentBlock = nullptr;
        currentOffset = 0;

        allocateNewBlock();

        _size = 0;
    }
    ~Arena()
    {
        for (void *block : blocks)
            std::free(block);

        _size = 0;
    }
    void allocateNewBlock();

    size_t _size;
    size_t blockSize;
    std::vector<void *> blocks;
    char *currentBlock;
    size_t currentOffset;
};

struct Object
{
    int type;
    bool marked;

    virtual ~Object() {}

    Object()
    {
        type = ObjectType::NIL;
        marked = false;
    }
    virtual bool operator==(const Object &other) const { return type == other.type; };
    virtual size_t hash() const { return std::hash<int>{}(type); }
    virtual std::string toString() { return "NIL"; }
};

struct Integer : Object
{
    Integer()
    {
        type = ObjectType::INT;
        marked = false;
    }
    ~Integer()
    {
        //  std::cout << "Free Integer" << std::endl;
    }

    bool operator==(const Object &other) const override
    {
        if (type != other.type)
            return false;
        if (auto o = dynamic_cast<const Integer *>(&other))
        {
            return this->value == o->value;
        }
        return false;
    }

    size_t hash() const override
    {
        return std::hash<int>{}(value);
    }

    std::string toString() override { return "Integer(" + std::to_string(value) + ")"; }
    int value{0};
};

struct Real : Object
{
    Real()
    {
        type = ObjectType::REAL;
        marked = false;
    }
    ~Real()
    {
        //  std::cout << "Free Real" << std::endl;
    }

    bool operator==(const Object &other) const override
    {
        if (type != other.type)
            return false;
        if (auto o = dynamic_cast<const Real *>(&other))
        {
            return this->value == o->value;
        }
        return false;
    }

    size_t hash() const override
    {
        return std::hash<double>{}(value);
    }

    std::string toString() override { return "Real(" + std::to_string(value) + ")"; }
    double value{0.0};
};

struct String : Object
{
    String()
    {
        type = ObjectType::STRING;
        marked = false;
    }
    ~String()
    {
        //   std::cout << "Free String" << std::endl;
    }
    bool operator==(const Object &other) const override
    {
        if (auto o = dynamic_cast<const String *>(&other))
        {
            return this->value == o->value;
        }
        return false;
    }

    size_t hash() const override
    {
        return std::hash<std::string>{}(value);
    }
    std::string toString() override { return "String(" + value + ")"; }
    std::string value;
};

struct Pointer : Object
{
    Pointer()
    {
        type = ObjectType::POINTER;
        marked = false;
    }
    ~Pointer()
    {
        //   std::cout << "Free Pointer" << std::endl;
    }

    bool operator==(const Object &other) const override
    {
        if (type != other.type)
            return false;
        if (auto o = dynamic_cast<const Pointer *>(&other))
        {
            return this->value == o->value;
        }
        return false;
    }

    size_t hash() const override
    {
        return std::hash<void *>{}(value);
    }

    std::string toString() override
    {
        return "Pointer";
    }

    size_t tag{0};
    void *value{nullptr};
};

struct List : Object
{
    List()
    {
        type = ObjectType::LIST;
        marked = false;
    }
    ~List()
    {
        std::cout << "Free List" << std::endl;
    }

    bool operator==(const Object &other) const override
    {
        if (type != other.type)
            return false;
        if (auto o = dynamic_cast<const List *>(&other))
        {
            return this->values.size() == o->values.size();
        }
        return false;
    }

    size_t hash() const override
    {
        size_t h = std::hash<size_t>{}(values.size());
        h += std::hash<size_t>{}(type);
        return h;
    }

    std::string toString() override { return "List"; }

    void add(Object *obj);
    Object *get(int index);
    bool find(Object *obj);
    bool remove(Object *obj);
    bool erase(int index);
    Object *pop();
    Object *back();
    int size() { return values.size(); }

    std::vector<Object *> values;
};

struct ObjectHash
{
    size_t operator()(const Object *obj) const
    {
        return obj->hash();
    }
};

struct ObjectEqual
{
    bool operator()(const Object *a, const Object *b) const
    {
        return *a == *b;
    }
};

struct Map : Object
{
    Map()
    {
        type = ObjectType::MAP;
        marked = false;
    }
    ~Map()
    {
        //   std::cout << "Free Map" << std::endl;
    }

    bool operator==(const Object &other) const override
    {
        if (type != other.type)
            return false;
        if (auto o = dynamic_cast<const Map *>(&other))
        {
            return this->values.size() == o->values.size();
        }
        return false;
    }

    size_t hash() const override
    {
        size_t h = std::hash<size_t>{}(values.size());
        h += std::hash<size_t>{}(type);
        return h;
    }

    std::string toString() override { return "Map"; }

    void insert(Object *key, Object *obj);
    bool contains(Object *key);
    void remove(Object *key);
    bool set(Object *key, Object *obj);
    Object *get(Object *key);

    std::unordered_map<Object *, Object *, ObjectHash, ObjectEqual> values;
};

struct Scope : Object
{
    Scope(Scope *parent)
    {
        this->parent = parent;
        type = ObjectType::SCOPE;
        marked = false;
    }
    virtual ~Scope()
    {
        values.clear();
        //  std::cout << "Free Scope" << std::endl;
    }

    void print()
    {
        for (auto it = values.begin(); it != values.end(); ++it)
        {
            std::cout << it->first << " : " << it->second->toString() << std::endl;
        }
    }
    bool define(const std::string &name, Object *obj)
    {
        values[name] = obj;
        return true;
    }

    bool remove(const std::string &name);

    bool define(const std::string &name, const std::string &value);
    bool define(const std::string &name, int value);
    bool define(const std::string &name, double value);
    bool define(const std::string &name);

    int getInt(const std::string &name);
    std::string getString(const std::string &name);
    double getReal(const std::string &name);

    Object *lookup(const std::string &name)
    {
        auto it = values.find(name);
        if (it != values.end())
            return it->second;
        if (parent != nullptr)
            return parent->lookup(name);
        return nullptr;
    }
    bool tryLookup(const std::string &name, Object **obj)
    {
        auto it = values.find(name);
        if (it != values.end())
        {
            *obj = it->second;
            return true;
        }
        if (parent != nullptr)
            return parent->tryLookup(name, obj);
        return false;
    }
    bool assign(const std::string &name, Object *obj)
    {
        auto it = values.find(name);
        if (it != values.end())
        {
            it->second = obj;
            return true;
        }
        if (parent != nullptr)
            return parent->assign(name, obj);
        return false;
    }

    Scope *parent = nullptr;
    std::string toString() override { return "Scope"; }
    std::unordered_map<std::string, Object *> values;
};

class Factory
{
public:
    static Factory &as()
    {
        static Factory factory;
        return factory;
    }
    void addRoot(Object *obj)
    {
        roots.insert(obj);
    }
    void removeRoot(Object *obj)
    {
        roots.erase(obj);
    }

    void mark();
    void sweep();

    void markValue(Object *obj)
    {
        obj->marked = true;
    }

    void collect()
    {
        mark();
        sweep();
    }

    void clean();

    Object *newNil()
    {
        void *p = Arena::as().allocate(sizeof(Object));
        Object *obj = new (p) Object();
        objects.push_back(obj);
        return obj;
    }

    Integer *newInteger(int value)
    {
        void *p = Arena::as().allocate(sizeof(Integer));
        Integer *obj = new (p) Integer();
        obj->value = value;
        objects.push_back(obj);
        return obj;
    }

    Real *newReal(double value)
    {
        void *p = Arena::as().allocate(sizeof(Real));
        Real *obj = new (p) Real();
        obj->value = value;
        objects.push_back(obj);
        return obj;
    }

    String *newString(const std::string &value)
    {
        void *p = Arena::as().allocate(sizeof(String));
        String *obj = new (p) String();
        obj->value = value;
        objects.push_back(obj);
        return obj;
    }

    Pointer *newPointer(size_t tag)
    {
        void *p = Arena::as().allocate(sizeof(Pointer));
        Pointer *obj = new (p) Pointer();
        obj->tag = tag;
        obj->value = nullptr;
        objects.push_back(obj);
        return obj;
    }

    List *newList()
    {
        void *p = Arena::as().allocate(sizeof(List));
        List *obj = new (p) List();
        objects.push_back(obj);
        return obj;
    }

    Map *newMap()
    {
        void *p = Arena::as().allocate(sizeof(Map));
        Map *obj = new (p) Map();
        objects.push_back(obj);
        return obj;
    }

    Scope *newScope(Scope *parent = nullptr)
    {
        void *p = Arena::as().allocate(sizeof(Scope));
        Scope *obj = new (p) Scope(parent);
        objects.push_back(obj);
        return obj;
    }
    void free(Object *obj);

    void setOnDelete(OnDeleteFunction function);

    size_t size() { return objects.size(); }

private:
    Factory();
    ~Factory();

    OnDeleteFunction onDelete;
    std::vector<Object *> objects;
    std::unordered_set<Object *> roots;
};

#define NEW_INTEGER(x) Factory::as().newInteger(x)
#define NEW_REAL(x) Factory::as().newReal(x)
#define NEW_STRING(x) Factory::as().newString(x)
#define NEW_POINTER(x) Factory::as().newPointer(x)
#define NEW_NIL() Factory::as().newNil()
#define NEW_LIST() Factory::as().newList()
#define NEW_MAP() Factory::as().newMap()
#define NEW_SCOPE(x) Factory::as().newScope(x)
#define ADD_ROOT(x) Factory::as().addRoot(x)
#define REMOVE_ROOT(x) Factory::as().removeRoot(x)