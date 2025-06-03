#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>

static const int LAYOUT_FR_LATIN1 = 0;
static const int LAYOUT_US_QWERTY = 1;

using namespace std;

enum class KeyCode {
    // Basic Latin
    KEY_A = 0, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
    KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
    KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X,
    KEY_Y, KEY_Z,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_SPACE, KEY_ENTER, KEY_BACKSPACE, KEY_TAB, KEY_ESC, KEY_MINUS, KEY_EQUAL,
    KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_BACKSLASH, KEY_SEMICOLON, KEY_APOSTROPHE,
    KEY_GRAVE, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3,
    KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_PRINTSCREEN, KEY_SCROLLLOCK, KEY_PAUSE, KEY_INSERT, KEY_HOME, KEY_PAGEUP,
    KEY_DELETE, KEY_END, KEY_PAGEDOWN, KEY_RIGHT, KEY_LEFT, KEY_DOWN, KEY_UP,

    // Extended Latin (ISO 8859-1)
    KEY_SQUARE, KEY_A_GRAVE, KEY_A_ACUTE, KEY_A_CIRCUMFLEX, KEY_A_TILDE, KEY_A_DIAERESIS, KEY_A_RING,
    KEY_C_CEDILLA, KEY_E_GRAVE, KEY_E_ACUTE, KEY_E_CIRCUMFLEX, KEY_E_DIAERESIS,
    KEY_I_GRAVE, KEY_I_ACUTE, KEY_I_CIRCUMFLEX, KEY_I_DIAERESIS, KEY_N_TILDE,
    KEY_O_GRAVE, KEY_O_ACUTE, KEY_O_CIRCUMFLEX, KEY_O_TILDE, KEY_O_DIAERESIS,
    KEY_U_GRAVE, KEY_U_ACUTE, KEY_U_CIRCUMFLEX, KEY_U_TILDE, KEY_U_DIAERESIS,
    KEY_Y_ACUTE, KEY_S_TILDE, KEY_O_SLASH, KEY_Y_DIAERESIS, KEY_EURO,

    // Dead Keys (for combining diacritics)
    DEAD_GRAVE, DEAD_ACUTE, DEAD_CIRCUMFLEX, DEAD_TILDE, DEAD_DIAERESIS, DEAD_CEDILLA,
    DEAD_MACRON, DEAD_BREVE, DEAD_ABOVEDOT, DEAD_ABOVERING, DEAD_DOUBLEACUTE, DEAD_CARON,
    DEAD_OGONEK, DEAD_BELOWDOT, DEAD_HOOK, DEAD_HORN,

    // For utility functions
    Count
};


namespace KeyUtils {
    KeyCode next(KeyCode k) {
        return static_cast<KeyCode>((static_cast<int>(k) + 1) % static_cast<int>(KeyCode::Count));
    }
}

const std::unordered_map<KeyCode, std::string> keyCodeMap = {
    // Basic Latin
    {KeyCode::KEY_A, "A"}, {KeyCode::KEY_B, "B"}, {KeyCode::KEY_C, "C"},
    {KeyCode::KEY_D, "D"}, {KeyCode::KEY_E, "E"}, {KeyCode::KEY_F, "F"},
    {KeyCode::KEY_G, "G"}, {KeyCode::KEY_H, "H"}, {KeyCode::KEY_I, "I"},
    {KeyCode::KEY_J, "J"}, {KeyCode::KEY_K, "K"}, {KeyCode::KEY_L, "L"},
    {KeyCode::KEY_M, "M"}, {KeyCode::KEY_N, "N"}, {KeyCode::KEY_O, "O"},
    {KeyCode::KEY_P, "P"}, {KeyCode::KEY_Q, "Q"}, {KeyCode::KEY_R, "R"},
    {KeyCode::KEY_S, "S"}, {KeyCode::KEY_T, "T"}, {KeyCode::KEY_U, "U"},
    {KeyCode::KEY_V, "V"}, {KeyCode::KEY_W, "W"}, {KeyCode::KEY_X, "X"},
    {KeyCode::KEY_Y, "Y"}, {KeyCode::KEY_Z, "Z"},
    {KeyCode::KEY_0, "0"}, {KeyCode::KEY_1, "1"}, {KeyCode::KEY_2, "2"},
    {KeyCode::KEY_3, "3"}, {KeyCode::KEY_4, "4"}, {KeyCode::KEY_5, "5"},
    {KeyCode::KEY_6, "6"}, {KeyCode::KEY_7, "7"}, {KeyCode::KEY_8, "8"},
    {KeyCode::KEY_9, "9"},
    {KeyCode::KEY_SPACE, "Space"}, {KeyCode::KEY_ENTER, "Enter"},
    {KeyCode::KEY_BACKSPACE, "Backspace"}, {KeyCode::KEY_TAB, "Tab"},
    {KeyCode::KEY_ESC, "Esc"}, {KeyCode::KEY_MINUS, "-"},
    {KeyCode::KEY_EQUAL, "="}, {KeyCode::KEY_LEFTBRACE, "["},
    {KeyCode::KEY_RIGHTBRACE, "]"}, {KeyCode::KEY_BACKSLASH, "\\"},
    {KeyCode::KEY_SEMICOLON, ";"}, {KeyCode::KEY_APOSTROPHE, "'"},
    {KeyCode::KEY_GRAVE, "`"}, {KeyCode::KEY_COMMA, ","},
    {KeyCode::KEY_DOT, "."}, {KeyCode::KEY_SLASH, "/"},
    {KeyCode::KEY_CAPSLOCK, "CapsLock"}, {KeyCode::KEY_F1, "F1"},
    {KeyCode::KEY_F2, "F2"}, {KeyCode::KEY_F3, "F3"}, {KeyCode::KEY_F4, "F4"},
    {KeyCode::KEY_F5, "F5"}, {KeyCode::KEY_F6, "F6"}, {KeyCode::KEY_F7, "F7"},
    {KeyCode::KEY_F8, "F8"}, {KeyCode::KEY_F9, "F9"}, {KeyCode::KEY_F10, "F10"},
    {KeyCode::KEY_F11, "F11"}, {KeyCode::KEY_F12, "F12"},
    {KeyCode::KEY_PRINTSCREEN, "PrintScreen"}, {KeyCode::KEY_SCROLLLOCK, "ScrollLock"},
    {KeyCode::KEY_PAUSE, "Pause"}, {KeyCode::KEY_INSERT, "Insert"},
    {KeyCode::KEY_HOME, "Home"}, {KeyCode::KEY_PAGEUP, "PageUp"},
    {KeyCode::KEY_DELETE, "Delete"}, {KeyCode::KEY_END, "End"},
    {KeyCode::KEY_PAGEDOWN, "PageDown"}, {KeyCode::KEY_RIGHT, "→"},
    {KeyCode::KEY_LEFT, "←"}, {KeyCode::KEY_DOWN, "↓"}, {KeyCode::KEY_UP, "↑"},

    // Extended Latin
    {KeyCode::KEY_A_GRAVE, "À"}, {KeyCode::KEY_A_ACUTE, "Á"},
    {KeyCode::KEY_A_CIRCUMFLEX, "Â"}, {KeyCode::KEY_A_TILDE, "Ã"},
    {KeyCode::KEY_A_DIAERESIS, "Ä"}, {KeyCode::KEY_A_RING, "Å"},
    {KeyCode::KEY_C_CEDILLA, "Ç"}, {KeyCode::KEY_E_GRAVE, "È"},
    {KeyCode::KEY_E_ACUTE, "É"}, {KeyCode::KEY_E_CIRCUMFLEX, "Ê"},
    {KeyCode::KEY_E_DIAERESIS, "Ë"}, {KeyCode::KEY_I_GRAVE, "Ì"},
    {KeyCode::KEY_I_ACUTE, "Í"}, {KeyCode::KEY_I_CIRCUMFLEX, "Î"},
    {KeyCode::KEY_I_DIAERESIS, "Ï"}, {KeyCode::KEY_N_TILDE, "Ñ"},
    {KeyCode::KEY_O_GRAVE, "Ò"}, {KeyCode::KEY_O_ACUTE, "Ó"},
    {KeyCode::KEY_O_CIRCUMFLEX, "Ô"}, {KeyCode::KEY_O_TILDE, "Õ"},
    {KeyCode::KEY_O_DIAERESIS, "Ö"}, {KeyCode::KEY_U_GRAVE, "Ù"},
    {KeyCode::KEY_U_ACUTE, "Ú"}, {KeyCode::KEY_U_CIRCUMFLEX, "Û"},
    {KeyCode::KEY_U_TILDE, "Ũ"}, {KeyCode::KEY_U_DIAERESIS, "Ü"},
    {KeyCode::KEY_Y_ACUTE, "Ý"}, {KeyCode::KEY_S_TILDE, "Ŝ"},
    {KeyCode::KEY_O_SLASH, "Ø"}, {KeyCode::KEY_Y_DIAERESIS, "Ÿ"},
    {KeyCode::KEY_EURO, "€"}, {KeyCode::KEY_SQUARE, "²"},

    // Dead keys
    {KeyCode::DEAD_GRAVE, "Dead `"}, {KeyCode::DEAD_ACUTE, "Dead ´"},
    {KeyCode::DEAD_CIRCUMFLEX, "Dead ^"}, {KeyCode::DEAD_TILDE, "Dead ~"},
    {KeyCode::DEAD_DIAERESIS, "Dead ¨"}, {KeyCode::DEAD_CEDILLA, "Dead ¸"},
    {KeyCode::DEAD_MACRON, "Dead ¯"}, {KeyCode::DEAD_BREVE, "Dead ˘"},
    {KeyCode::DEAD_ABOVEDOT, "Dead ˙"}, {KeyCode::DEAD_ABOVERING, "Dead ˚"},
    {KeyCode::DEAD_DOUBLEACUTE, "Dead ˝"}, {KeyCode::DEAD_CARON, "Dead ˇ"},
    {KeyCode::DEAD_OGONEK, "Dead ˛"}, {KeyCode::DEAD_BELOWDOT, "Dead ̣"},
    {KeyCode::DEAD_HOOK, "Dead ̉"}, {KeyCode::DEAD_HORN, "Dead ̛"}
};

std::string keyCodeToString(KeyCode code) {
    auto it = keyCodeMap.find(code);
    return (it != keyCodeMap.end()) ? it->second : "Unknown";
}

std::optional<KeyCode> stringToKeyCode(const std::string& str) {
    static std::unordered_map<std::string, KeyCode> reverseMap;

    // Populate the reverse map only once
    if (reverseMap.empty()) {
        for (const auto& [code, name] : keyCodeMap) {
            reverseMap[name] = code;
        }
    }

    auto it = reverseMap.find(str);
    if (it != reverseMap.end()) {
        return it->second;
    } else {
        return std::nullopt; // not found
    }
}

class Key {
    public:
        KeyCode key;
        Key(KeyCode k) {
            key = k;
        }
};

// Represents a row of key
class Row {
    public:
        vector<Key> keys;

        void addKeys(vector<Key> k) {
            for (const auto key : k) {
                keys.emplace_back(key);
            }
        }

        string print() const {
            string output;
            for (const auto& key : keys) {
                output += "[" + keyCodeToString(key.key) + "] ";
            }
            return output;
        }
};

// Represents the whole keyboard layout
class KeyboardLayout {
private:
    vector<Row> rows;

public:
    void addRow(const Row& row) {
        rows.push_back(row);
    }

    string display() const {
        string output;
        for (const auto& row : rows) {
            output += row.print() + "\n";
        }
        return output;
        // TODO: add a way to display the keyboard layout in a more user-friendly way, like a grid or something like that, or even a GUI if y
    }
};

class Keyboard {
    public:
        static KeyboardLayout get(int layout) {
            KeyboardLayout output;

            switch(layout) {
                case LAYOUT_US_QWERTY:
                    {
                        Row row1;
                        row1.addKeys({Key(KeyCode::KEY_Q), Key(KeyCode::KEY_W), Key(KeyCode::KEY_E), Key(KeyCode::KEY_R), Key(KeyCode::KEY_T), Key(KeyCode::KEY_Y), Key(KeyCode::KEY_U), Key(KeyCode::KEY_I), Key(KeyCode::KEY_O), Key(KeyCode::KEY_P)});
                        Row row2;
                        row2.addKeys({Key(KeyCode::KEY_A), Key(KeyCode::KEY_S), Key(KeyCode::KEY_D), Key(KeyCode::KEY_F), Key(KeyCode::KEY_G), Key(KeyCode::KEY_H), Key(KeyCode::KEY_J), Key(KeyCode::KEY_K), Key(KeyCode::KEY_L)});
                        Row row3;
                        row3.addKeys({Key(KeyCode::KEY_Z), Key(KeyCode::KEY_X), Key(KeyCode::KEY_C), Key(KeyCode::KEY_V), Key(KeyCode::KEY_B), Key(KeyCode::KEY_N), Key(KeyCode::KEY_M)});
                        output.addRow(row1);
                        output.addRow(row2);
                        output.addRow(row3);
                    }
                    break;
                case LAYOUT_FR_LATIN1:
                    /*
                    {"Esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Pause", "Impr.E", "Inser", "Suppr"},
                    {"²", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "°", "+", "ret", "arrow", NULL, NULL, NULL},
                    {"⇄", "A", "Z", "E", "R", "T", "Y", "U", "I", "O", "P", "-", "$", "Enter", "Page up", NULL, NULL, NULL},
                    {"🔒", "Q", "S", "D", "F", "G", "H", "J", "K", "L", "M", "%", "µ", "Page down", NULL, NULL, NULL, NULL},
                    {"↑", "Chev", "W", "X", "C", "V", "B", "N", "?", ".", "/", "!", "Shift", "Fin", NULL, NULL, NULL, NULL},
                    {"ctrl", "Fn", "Super", "Alt", "Space", "Alt Gr", "Menu", "Ctrl", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
                    */
                    {
                        Row row1;
                        row1.addKeys({Key(KeyCode::KEY_ESC)});
                        KeyCode f = KeyCode::KEY_F1;
                        for (int i = 0; i < 12; i++) {
                            row1.addKeys({Key(f)});
                            f = KeyUtils::next(f);
                        }
                        //, Key(KeyCode::KEY_F1), Key(KeyCode::KEY_F2), Key(KeyCode::KEY_F3), Key(KeyCode::KEY_T), Key(KeyCode::KEY_Y), Key(KeyCode::KEY_U), Key(KeyCode::KEY_I), Key(KeyCode::KEY_O), Key(KeyCode::KEY_P)});
                        Row row2;
                        row2.addKeys({Key(KeyCode::KEY_SQUARE)});
                        KeyCode n = KeyCode::KEY_1;
                        for (int i = 0; i < 9; i++) {
                            row2.addKeys({Key(n)});
                            n = KeyUtils::next(n);
                        }
                        row2.addKeys({Key(KeyCode::KEY_0)});
                        Row row3;
                        row3.addKeys({Key(KeyCode::KEY_A), Key(KeyCode::KEY_S), Key(KeyCode::KEY_D), Key(KeyCode::KEY_F), Key(KeyCode::KEY_G), Key(KeyCode::KEY_H), Key(KeyCode::KEY_J), Key(KeyCode::KEY_K), Key(KeyCode::KEY_L)});
                        Row row4;
                        row4.addKeys({Key(KeyCode::KEY_Z), Key(KeyCode::KEY_X), Key(KeyCode::KEY_C), Key(KeyCode::KEY_V), Key(KeyCode::KEY_B), Key(KeyCode::KEY_N), Key(KeyCode::KEY_M)});
                        output.addRow(row1);
                        output.addRow(row2);
                        output.addRow(row3);
                        output.addRow(row4);
                    }
                    break;
            }
            return output;
        }
};

int main() {
    KeyboardLayout layout = Keyboard::get(LAYOUT_FR_LATIN1);
    std::cout << layout.display() << std::endl;

    layout = Keyboard::get(LAYOUT_US_QWERTY);
    std::cout << layout.display() << std::endl;
    return 0;
}