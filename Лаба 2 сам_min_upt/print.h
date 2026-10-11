#pragma once

#include "include/Sequence.h"
#include "util.h"

#include <iostream>

inline void print_sequence_kind_menu() {
    std::cout << "\nChoose sequence type:\n";
    std::cout << "1. MutableArraySequence\n";
    std::cout << "2. ImmutableArraySequence\n";
    std::cout << "3. MutableListSequence\n";
    std::cout << "4. ImmutableListSequence\n";
}

inline const char *sequence_kind_name(int kind) {
    switch (kind) {
        case 1:
            return "MutableArraySequence";
        case 2:
            return "ImmutableArraySequence";
        case 3:
            return "MutableListSequence";
        case 4:
            return "ImmutableListSequence";
        default:
            return "Unknown";
    }
}

inline void print_sequence(const Sequence<int> &sequence) {
    std::cout << "[";
    for (int index = 0; index < sequence.get_length(); ++index) {
        if (index > 0) {
            std::cout << ", ";
        }
        std::cout << sequence.get(index);
    }
    std::cout << "]";
}

inline void print_sequence(const char *name, const Sequence<int> &sequence) {
    std::cout << name << " = ";
    print_sequence(sequence);
    std::cout << "\n";
}

inline void print_current_sequence(const Sequence<int> &sequence, int kind) {
    std::cout << sequence_kind_name(kind) << " length=" << sequence.get_length() << " value=";
    print_sequence(sequence);
    std::cout << "\n";
}

inline void print_option(const Option<int> &option) {
    if (option.has_value()) {
        std::cout << "Value: " << option.get_value() << "\n";
    } else {
        std::cout << "No value\n";
    }
}

inline void print_option(const char *name, const Option<int> &option) {
    std::cout << name << " = ";
    if (option.has_value()) {
        std::cout << option.get_value();
    } else {
        std::cout << "None";
    }
    std::cout << "\n";
}

inline void print_with_enumerator(const Sequence<int> &sequence) {
    IEnumerator<int> *enumerator = sequence.get_enumerator();

    std::cout << "[";
    bool first = true;
    while (enumerator->move_next()) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << enumerator->get_curr();
        first = false;
    }
    std::cout << "]\n";

    delete enumerator;
}

inline void print_with_enumerator(const char *name, const Sequence<int> &sequence) {
    std::cout << name << " = ";
    print_with_enumerator(sequence);
}

inline void print_operation_menu() {
    std::cout << "\nMenu:\n";
    std::cout << "1. Print sequence\n";
    std::cout << "2. Get first\n";
    std::cout << "3. Get last\n";
    std::cout << "4. Get by index\n";
    std::cout << "5. Try get by index\n";
    std::cout << "6. Append\n";
    std::cout << "7. Prepend\n";
    std::cout << "8. Insert\n";
    std::cout << "9. Remove\n";
    std::cout << "10. Get subsequence\n";
    std::cout << "11. Concat\n";
    std::cout << "12. Map square\n";
    std::cout << "13. Where even\n";
    std::cout << "14. Reduce sum\n";
    std::cout << "15. Print with enumerator\n";
    std::cout << "16. Create new sequence\n";
    std::cout << "0. Exit\n";
}

inline void print_subsequence(const Sequence<int> &sequence) {
    int start_index = read_int("Start index: ");
    int end_index = read_int("End index: ");

    Sequence<int> *result = sequence.get_subsequence(start_index, end_index);
    std::cout << "Result: ";
    print_sequence(*result);
    std::cout << "\n";
    delete result;
}

inline void print_mapped(Sequence<int> &sequence) {
    Sequence<int> *result = sequence.map(square);
    std::cout << "Result: ";
    print_sequence(*result);
    std::cout << "\n";
    delete result;
}

inline void print_filtered(Sequence<int> &sequence) {
    Sequence<int> *result = sequence.where(is_even);
    std::cout << "Result: ";
    print_sequence(*result);
    std::cout << "\n";
    delete result;
}
