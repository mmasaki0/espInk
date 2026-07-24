#pragma once

#include <unordered_map>
#include <vector>

struct song {
    char path[256];
    song() {}
    song(const char *p) {
        strncpy(path, p, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    };
};

namespace trie {
    struct Range {
        int lower;
        int upper;

        int calculateRange() {
            return upper - lower + 1;
        }

        bool contains(int i) {
            return i >= lower && i <= upper;
        }

    };

    typedef std::vector<Range> charset;

    struct node {
        bool exists;
        struct node** children;
    };

    class Trie {
    public:
        Trie(charset chars)
            : chars(chars), numChars(0), root(nullptr) {
            
                for (Range r : chars) {
                    numChars += r.calculateRange();
                }

                root = new node;
                root->exists = false;
                root->children = new node*[numChars];

                for (int i = 0; i < numChars; i++) {
                    root->children[i] = NULL;
                }
            }

        bool insert(std::string key) {
            int idx;
            node* current = root;

            for (char c : key) {
                idx = getIdx(c);
                if(idx == -1) {
                    return false;
                }
                if(!current->children[idx] || current->children[idx] == NULL) {
                    current = new node;
                    current->exists = false;
                    current->children = new node*[numChars];

                    for (int i = 0; i < numChars; i++) {
                        current->children[i] = NULL;
                    }
                }
                current = current->children[idx];
            }

            current->exists = true;
            return true; 
        }

        void cleanup() {
            unloadNode(root);
        }
    private:
        charset chars;
        unsigned int numChars;
        node* root;

        int getIdx(char c) {
            int ret = 0;

            for(Range r: chars) {
                if(r.contains((int)c)) {
                    ret += int(c) - r.lower;
                    break;
                } else {
                    ret += r.calculateRange();
                }
            }

            return ret == numChars ? -1 : ret;
        }

        void unloadNode(node* top) {
            if(!top) {
                return;
            }

            for(int i = 0; i < numChars; i++) {
                if(top->children[i]) {
                    unloadNode(top->children[i]);
                }
            }

            top = nullptr;
        }
    };
};

extern std::unordered_map<uint16_t, song> mapLibrary;