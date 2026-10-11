#include "print.h"
#include "util.h"

#include <exception>
#include <iostream>

void execute_command(Sequence<int> *&sequence, int &kind, int command, bool &running) {
    switch (command) {
        case 0:
            running = false;
            return;
        case 1:
            print_current_sequence(*sequence, kind);
            return;
        case 2:
            std::cout << "First: " << sequence->get_first() << "\n";
            return;
        case 3:
            std::cout << "Last: " << sequence->get_last() << "\n";
            return;
        case 4: {
            int index = read_int("Index: ");
            std::cout << "Value: " << sequence->get(index) << "\n";
            return;
        }
        case 5: {
            int index = read_int("Index: ");
            print_option(sequence->try_get(index));
            return;
        }
        case 6:
            append_value(sequence);
            return;
        case 7:
            prepend_value(sequence);
            return;
        case 8:
            insert_value(sequence);
            return;
        case 9:
            remove_value(sequence);
            return;
        case 10:
            print_subsequence(*sequence);
            return;
        case 11:
            concat_sequence(sequence);
            return;
        case 12:
            print_mapped(*sequence);
            return;
        case 13:
            print_filtered(*sequence);
            return;
        case 14:
            std::cout << "Sum: " << sequence->reduce(sum_values, 0) << "\n";
            return;
        case 15:
            print_with_enumerator(*sequence);
            return;
        case 16:
            reset_sequence(sequence, kind);
            return;
        default:
            std::cout << "Unknown command\n";
            return;
    }
}

int main() {
    Sequence<int> *sequence = nullptr;
    int kind = 0;

    std::cout << "Sequence console UI\n";
    reset_sequence(sequence, kind);

    bool running = true;
    while (running) {
        print_current_sequence(*sequence, kind);
        print_operation_menu();
        int command = read_int("Command: ");

        try {
            execute_command(sequence, kind, command, running);
        } catch (const std::exception &error) {
            std::cout << "Error: " << error.what() << "\n";
        }
    }

    delete sequence;
    std::cout << "Bye\n";
    return 0;
}
