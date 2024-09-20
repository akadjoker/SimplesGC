#include "pch.h"
#include "Garbage.hpp"
#include <time.h>

size_t GC_DYNAMIC_THRESHOLD = 2024*2;
clock_t lastCollectTime = 0;
const size_t collectFrequency = 100;

size_t adjustThreshold()
{
    clock_t currentTime = clock();
    double elapsedTime = (double)(currentTime - lastCollectTime) * 1000.0 / CLOCKS_PER_SEC;

    if (elapsedTime < collectFrequency)
    {

        lastCollectTime = currentTime;
        return GC_DYNAMIC_THRESHOLD * 1.5;
    }
    else
    {

        lastCollectTime = currentTime;
        return GC_DYNAMIC_THRESHOLD / 1.2;
    }
}

void *Arena::allocate(size_t size)
{

     if (currentOffset + size > blockSize) 
     {
        allocateNewBlock();   
    }

    void* p = currentBlock + currentOffset;  
    currentOffset += size;  
    this->_size += size;

    if (this->_size > GC_DYNAMIC_THRESHOLD)
    {
        Factory::as().collect();
        GC_DYNAMIC_THRESHOLD = adjustThreshold();
    }

    return p;
}

void Arena::free(void *p, size_t size)
{
    this->_size -= size;
   // std::free(p);
}

void Arena::allocateNewBlock()
{
    currentBlock = static_cast<char *>(std::malloc(blockSize)); 
    if (currentBlock)
    {
        blocks.push_back(currentBlock); 
        currentOffset = 0;             
    }
}

//**************************************************************************** */
// scope

bool Scope::remove(const std::string &name)
{
    return values.erase(name);
}

bool Scope::define(const std::string &name, const std::string &value)
{
    Object *obj = Factory::as().newString(value);
    return define(name, obj);
}

bool Scope::define(const std::string &name, int value)
{
    Object *obj = Factory::as().newInteger(value);
    return define(name, obj);
}

bool Scope::define(const std::string &name, double value)
{
    Object *obj = Factory::as().newReal(value);
    return define(name, obj);
}

bool Scope::define(const std::string &name)
{
    Object *obj = Factory::as().newNil();
    return define(name, obj);
}

int Scope::getInt(const std::string &name)
{
    Object *obj;
    if (tryLookup(name, &obj))
    {
        if (obj->type == ObjectType::INT)
        {
            return static_cast<Integer *>(obj)->value;
        }
    }
    std::cout << "Not an integer" << std::endl;
    return 0;
}

double Scope::getReal(const std::string &name)
{
    Object *obj;
    if (tryLookup(name, &obj))
    {
        if (obj->type == ObjectType::REAL)
        {
            return static_cast<Real *>(obj)->value;
        }
    }
    return 0;
}

std::string Scope::getString(const std::string &name)
{
    Object *obj;
    if (tryLookup(name, &obj))
    {
        if (obj->type == ObjectType::STRING)
        {
            return static_cast<String *>(obj)->value;
        }
    }
    return "";
}

//**************************************************************************** */
// Factory

// void Factory::mark()
// {
//     if (roots.empty())
//     {
//         std::cout << "Nothing to mark" << std::endl;
//         return;
//     }

//     std::cout << "Total objects: " << roots.size() << " to mark" << std::endl;
//     std::vector<Object *> worklist(roots.begin(), roots.end());
//     while (!worklist.empty())
//     {
//         auto obj = worklist.back();
//         worklist.pop_back();

//         if (!obj->marked)
//         {
//             obj->marked = true;
//             if (obj->type == ObjectType::SCOPE)
//             {
//                 Scope *scope = static_cast<Scope *>(obj);
//                 for (auto it = scope->values.begin(); it != scope->values.end(); ++it)
//                 {
//                     worklist.insert(worklist.end(), it->second);
//                 }
//                 if (scope->parent != nullptr)
//                     worklist.insert(worklist.end(), scope->parent);
//             }
//         }
//     }
// }

/// fifo

void Factory::mark()
{
    if (roots.empty())
    {
        std::cout << "Nothing to mark" << std::endl;
        return;
    }

    //  std::cout << "Total objects: " << roots.size() << " to mark" << std::endl;
    std::deque<Object *> worklist(roots.begin(), roots.end()); //

    while (!worklist.empty())
    {
        Object *obj = worklist.front();
        worklist.pop_front();

        if (!obj->marked)
        {
            obj->marked = true;

            if (obj->type == ObjectType::SCOPE)
            {
                Scope *scope = static_cast<Scope *>(obj);

                for (auto &it : scope->values)
                {
                    if (!it.second->marked)
                    {
                        worklist.push_back(it.second);
                    }
                }

                if (scope->parent != nullptr && !scope->parent->marked)
                {
                    worklist.push_back(scope->parent);
                }
            }
            else if (obj->type == ObjectType::LIST)
            {
                List *list = static_cast<List *>(obj);
                //  std::cout << "List " << list->size() << std::endl;
                for (int i = 0; i < list->size(); i++)
                {
                    Object *value = list->get(i);
                    worklist.push_back(value);
                }
            }
        }
    }
}

void Factory::sweep()
{
    if (objects.empty())
    {
        std::cout << "Nothing to collect" << std::endl;
        return;
    }
    //    std::cout << "Total objects: " << objects.size() << " to collect" << std::endl;
    //   std::cout << "Total memory used: " << Arena::as().size() << " bytes." << std::endl;

    auto it = objects.begin();
    while (it != objects.end())
    {
        auto object = *it;
        if (object->marked)
        {
            object->marked = false;
            ++it;
        }
        else
        {
            it = objects.erase(it);
            // std::cout << "GC " << object->toString() << std::endl;
            this->free(object);
        }
    }
}

void Factory::clean()
{
    if (objects.empty())
    {
        std::cout << "Nothing to clean" << std::endl;
        return;
    }

    std::cout << "Total objects: " << objects.size() << " to clean" << std::endl;
    if (!objects.empty())
    {
        for (auto &obj : objects)
        {
            free(obj);
        }
    }
    objects.clear();
}

void Factory::free(Object *obj)
{
    if (obj->type == ObjectType::NIL)
    {
        Object *o = static_cast<Object *>(obj);
        o->~Object();
        Arena::as().free(o, sizeof(Object));
    }
    else if (obj->type == ObjectType::INT)
    {
        Integer *i = static_cast<Integer *>(obj);
        i->~Integer();
        Arena::as().free(i, sizeof(Integer));
    }
    else if (obj->type == ObjectType::REAL)
    {
        Real *r = static_cast<Real *>(obj);
        r->~Real();
        Arena::as().free(r, sizeof(Real));
    }
    else if (obj->type == ObjectType::STRING)
    {
        String *s = static_cast<String *>(obj);
        s->~String();
        Arena::as().free(s, sizeof(String));
    }
    else if (obj->type == ObjectType::POINTER)
    {
        Pointer *p = static_cast<Pointer *>(obj);
        onDelete(p);
        p->value = nullptr;
        p->~Pointer();
        Arena::as().free(p, sizeof(Pointer));
    }
    else if (obj->type == ObjectType::LIST)
    {
        List *l = static_cast<List *>(obj);
        l->~List();
        Arena::as().free(l, sizeof(List));
    }
    else if (obj->type == ObjectType::MAP)
    {
        Map *m = static_cast<Map *>(obj);
        m->~Map();
        Arena::as().free(m, sizeof(Map));
    }
    else if (obj->type == ObjectType::SCOPE)
    {
        Scope *s = static_cast<Scope *>(obj);
        s->~Scope();
        Arena::as().free(s, sizeof(Scope));
    }
    else
        std::cout << "Unknown object type" << std::endl;
}

static void defaultOnDelete(Pointer *obj)
{
    (void)obj;
}

void Factory::setOnDelete(OnDeleteFunction function)
{
    if (function != nullptr)
        onDelete = function;
    else
        onDelete = defaultOnDelete;
}

Factory::Factory()
{
    onDelete = defaultOnDelete;
    objects.reserve(GC_THRESHOLD);
}

Factory::~Factory()
{
    clean();
}

void List::add(Object *obj)
{
    values.push_back(obj);
}

Object *List::get(int index)
{
    if (values.empty())
        return nullptr;
    if (index < 0 || index >= (int)values.size())
    {
        std::cout << "Index out [" << index << "] of bounds" << std::endl;
        return nullptr;
    }
    return values[index];
}

bool List::find(Object *obj)
{
    auto it = values.begin();
    while (it != values.end())
    {
        if (*it == obj)
            return true;
        it++;
    }
    return false;
}

bool List::remove(Object *obj)
{
    auto it = values.begin();
    while (it != values.end())
    {
        if (*it == obj)
        {
            values.erase(it);
            return true;
        }
        it++;
    }
    return false;
}

bool List::erase(int index)
{
    if (index < 0 || index >= (int)values.size())
    {
        std::cout << "Index out [" << index << "] of bounds" << std::endl;
        return false;
    }
    values.erase(values.begin() + index);
    return true;
}

Object *List::pop()
{
    Object *value = values.back();
    values.pop_back();
    return value;
}

Object *List::back()
{
    return values.back();
}

void Map::insert(Object *key, Object *obj)
{
    values[key] = obj;
}

bool Map::contains(Object *key)
{
    return values.find(key) != values.end();
}

void Map::remove(Object *key)
{
    values.erase(key);
}

bool Map::set(Object *key, Object *obj)
{
    auto it = values.find(key);
    if (it != values.end())
    {
        it->second = obj;
        return true;
    }
    return false;
}

Object *Map::get(Object *key)
{
    auto it = values.find(key);
    if (it != values.end())
        return it->second;
    return nullptr;
}
