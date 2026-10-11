#include <iostream>
#include <type_traits>
#include <utility>

#include "include/UniquePtr.h"
#include "include/SharedPtr.h"
#include "Лаба 2 сам_min_upt/include/ArraySequence.h"

namespace {
int passed = 0;
int failed = 0;

void check(bool condition, const char* name) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failed;
        std::cout << "[FAIL] " << name << '\n';
    }
}

struct Tracked {
    static int alive;
    int value;

    explicit Tracked(int value = 0) : value(value) { ++alive; }
    Tracked(const Tracked& other) : value(other.value) { ++alive; }
    ~Tracked() { --alive; }
};
int Tracked::alive = 0;

struct C2 {
    static int destroyed;
    virtual ~C2() { ++destroyed; }
    virtual C2* clone() const = 0;
    virtual int value() const = 0;
};
int C2::destroyed = 0;

struct C1 : C2 {
    static int destroyed;
    int number;

    explicit C1(int number) : number(number) {}
    ~C1() override { ++destroyed; }
    C1* clone() const override { return new C1(*this); }
    int value() const override { return number; }
};
int C1::destroyed = 0;

void test_unique_object() {
    {
        UniquePtr<Tracked> first(new Tracked(10));
        UniquePtr<Tracked> copy(first);
        check(first.get() != copy.get() && copy->value == 10,
              "UniquePtr copies an independent object");

        copy->value = 20;
        check(first->value == 10, "UniquePtr copy does not change source");

        first = copy;
        check(first.get() != copy.get() && first->value == 20,
              "UniquePtr copy assignment replaces the object");

        UniquePtr<Tracked> moved(std::move(first));
        check(first.get() == nullptr && moved->value == 20,
              "UniquePtr move constructor transfers ownership");

        first = std::move(moved);
        check(moved.get() == nullptr && first->value == 20,
              "UniquePtr move assignment transfers ownership");

        Tracked* released = first.release();
        check(first.get() == nullptr && released->value == 20,
              "UniquePtr release returns the object");
        first.reset(released);
        first.reset();
        check(first.get() == nullptr && Tracked::alive == 1,
              "UniquePtr reset destroys only its own object");
    }
    check(Tracked::alive == 0, "UniquePtr leaves no live objects");
}

void test_unique_array() {
    static_assert(!std::is_copy_constructible_v<UniquePtr<Tracked[]>>);
    {
        UniquePtr<Tracked[]> array(new Tracked[3]);
        check(Tracked::alive == 3, "UniquePtr array owns all elements");
        array[1].value = 42;

        UniquePtr<Tracked[]> moved(std::move(array));
        check(array.get() == nullptr && moved[1].value == 42,
              "UniquePtr array moves and supports indexing");

        array = std::move(moved);
        check(moved.get() == nullptr && array[1].value == 42,
              "UniquePtr array move assignment transfers ownership");
        array.reset(new Tracked[2]);
        check(Tracked::alive == 2, "UniquePtr array reset deletes the old array");
    }
    check(Tracked::alive == 0, "UniquePtr array deletes every element");
}

void test_unique_subtyping() {
    C2::destroyed = 0;
    C1::destroyed = 0;
    {
        UniquePtr<C1> child(new C1(7));
        UniquePtr<C2> copy(child);
        check(child.get() != copy.get() && copy->value() == 7,
              "UniquePtr C1 to C2 copy clones the object");

        UniquePtr<C2> moved(std::move(child));
        check(child.get() == nullptr && moved->value() == 7,
              "UniquePtr C1 to C2 move transfers ownership");

        UniquePtr<C1> replacement(new C1(9));
        moved = std::move(replacement);
        check(replacement.get() == nullptr && moved->value() == 9,
              "UniquePtr C1 to C2 move assignment works");
    }
    check(C1::destroyed == 3 && C2::destroyed == 3,
          "UniquePtr destroys every C1 object exactly once");
}

void test_shared_object() {
    {
        SharedPtr<Tracked> first(new Tracked(5));
        SharedPtr<Tracked> copy(first);
        check(first.get() == copy.get() && Tracked::alive == 1,
              "SharedPtr copy shares one object");

        first.reset();
        check(copy.get() != nullptr && Tracked::alive == 1,
              "SharedPtr reset keeps an object with another owner");

        SharedPtr<Tracked> target(new Tracked(8));
        target = copy;
        check(Tracked::alive == 1 && target.get() == copy.get(),
              "SharedPtr copy assignment releases old ownership");

        SharedPtr<Tracked> moved(std::move(copy));
        check(copy.get() == nullptr && moved.get() == target.get(),
              "SharedPtr move constructor transfers ownership");

        target = std::move(moved);
        check(moved.get() == nullptr && target->value == 5,
              "SharedPtr move assignment transfers ownership");

        SharedPtr<Tracked>& self = target;
        target = self;
        target = std::move(self);
        check(target.get() != nullptr && Tracked::alive == 1,
              "SharedPtr self-assignment preserves ownership");
    }
    check(Tracked::alive == 0, "SharedPtr deletes the last shared object");
}

void test_shared_subtyping() {
    C2::destroyed = 0;
    C1::destroyed = 0;
    {
        SharedPtr<C1> child(new C1(11));
        SharedPtr<C2> base(child);
        check(child.get() == base.get() && base->value() == 11,
              "SharedPtr C1 to C2 copy shares ownership");

        child.reset();
        check(base->value() == 11 && C1::destroyed == 0,
              "SharedPtr C2 keeps C1 object alive");

        SharedPtr<C1> fourth(new C1(14));
        SharedPtr<C2> assigned;
        assigned = fourth;
        fourth.reset();
        check(assigned->value() == 14 && C1::destroyed == 0,
              "SharedPtr C1 to C2 copy assignment shares ownership");

        SharedPtr<C1> fifth(new C1(15));
        SharedPtr<C2> moved(std::move(fifth));
        check(fifth.get() == nullptr && moved->value() == 15,
              "SharedPtr C1 to C2 move constructor transfers ownership");

        SharedPtr<C1> sixth(new C1(16));
        assigned = std::move(sixth);
        check(sixth.get() == nullptr && assigned->value() == 16 && C1::destroyed == 1,
              "SharedPtr C1 to C2 move assignment transfers ownership");

        SharedPtr<C1> seventh(new C1(17));
        SharedPtr<C2> same_group(seventh);
        same_group = std::move(seventh);
        check(seventh.get() == nullptr && same_group->value() == 17,
              "SharedPtr move assignment preserves a shared ownership group");
    }
    check(C1::destroyed == 5 && C2::destroyed == 5,
          "SharedPtr destroys every C1 object exactly once");
}

void test_array_sequence() {
    {
        SharedPtr<Tracked> input[] = {
            SharedPtr<Tracked>(new Tracked(1)),
            SharedPtr<Tracked>(new Tracked(2))
        };
        MutableArraySequence<SharedPtr<Tracked>> sequence(input, 2);
        check(sequence.get_length() == 2 && sequence.get(0).get() == input[0].get(),
              "ArraySequence stores SharedPtr elements");

        sequence.append(SharedPtr<Tracked>(new Tracked(3)));
        check(sequence.get_length() == 3 && sequence.get(2)->value == 3,
              "ArraySequence grows through UniquePtr array storage");

        UniquePtr<Sequence<SharedPtr<Tracked>>> part(sequence.get_subsequence(1, 2));
        check(part->get_length() == 2 && part->get(0)->value == 2 &&
                  part->get(1).get() == sequence.get(2).get(),
              "ArraySequence subsequence preserves shared ownership");

        input[0].reset();
        check(sequence.get(0)->value == 1 && Tracked::alive == 3,
              "ArraySequence keeps objects alive after external reset");
    }
    check(Tracked::alive == 0, "ArraySequence releases all stored objects");
}

} // namespace

int main() {
    test_unique_object();
    test_unique_array();
    test_unique_subtyping();
    test_shared_object();
    test_shared_subtyping();
    test_array_sequence();

    std::cout << "\nPassed: " << passed << "\nFailed: " << failed << '\n';
    if (failed != 0) {
        return 1;
    }
    return 0;
}
