// ZoneHistory.h  --  AgriSense AI  --  Programming Lab (Data Layer)
// Operations:
//   - insertEvent()    : append at tail (chronological order)     O(1)
//   - getOldest()      : head  (oldest-first traversal)           O(n)
//   - getNewest()      : tail  (newest-first traversal)           O(n)
//   - getFromNewest(k) : walk backwards k steps                   O(k)
#ifndef ZONEHISTORY_H
#define ZONEHISTORY_H

#include <cstring>
#include <cstdio>

using namespace std;

// Node -- one entry in the maintenance log

struct HistoryNode {
    char category[48];
    char date[32];
    char detail[64];
    char note[64];
    HistoryNode* prev;
    HistoryNode* next;

    // Constructor: null the pointers, empty the strings
    HistoryNode() : prev(NULL), next(NULL) {
        category[0] = date[0] = detail[0] = note[0] = '\0';
    }
};

// -----------------------------------------------------------------------------
// ZoneHistory -- the doubly-linked list itself
// -----------------------------------------------------------------------------
class ZoneHistory {
private:
    HistoryNode* head;    // oldest
    HistoryNode* tail;    // newest
    int          count;   // O(1) size lookup

    // Free all nodes and reset the list to empty
    void clear() {
        HistoryNode* cur = head;
        while (cur) {
            HistoryNode* nx = cur->next;   // save BEFORE delete
            delete cur;
            cur = nx;
        }
        head = tail = NULL;
        count = 0;
    }

public:
    //Default constructor
    ZoneHistory() : head(NULL), tail(NULL), count(0) {}

    //Rule of 3: copy constructor 
    ZoneHistory(const ZoneHistory& other) : head(NULL), tail(NULL), count(0) {
        for (HistoryNode* n = other.head; n; n = n->next)
            insertEvent(n->category, n->date, n->detail, n->note);
    }

    //Rule of 3: copy assignment 
    ZoneHistory& operator=(const ZoneHistory& other) {
        if (this == &other) return *this;   // self-assignment guard
        clear();
        for (HistoryNode* n = other.head; n; n = n->next)
            insertEvent(n->category, n->date, n->detail, n->note);
        return *this;
    }

    //Rule of 3: destructor
    ~ZoneHistory() { clear(); }

    // insertEvent -- append one entry at the tail.  O(1) because we keep tail.
    
    void insertEvent(const char* category, const char* date,
                     const char* detail,   const char* note) {
        HistoryNode* node = new HistoryNode();

        // Safe copy leave room for the null terminator
        strncpy(node->category, category ? category : "", sizeof(node->category) - 1);
        strncpy(node->date,     date     ? date     : "", sizeof(node->date)     - 1);
        strncpy(node->detail,   detail   ? detail   : "", sizeof(node->detail)   - 1);
        strncpy(node->note,     note     ? note     : "", sizeof(node->note)     - 1);

        if (!tail) {
            head = tail = node;           // first node
        } else {
            tail->next = node;            // old tail -> new node
            node->prev = tail;            // new node -> old tail
            tail       = node;            // update tail
        }
        ++count;
    }

    // ---- Traversal entry points ----
    HistoryNode* getOldest() const { return head; }
    HistoryNode* getNewest() const { return tail; }

    // ---- Accessors ----
    int  getCount() const { return count; }
    bool isEmpty()  const { return count == 0; }

    // -------------------------------------------------------------------------
    // getFromNewest(k) -- 0 = newest, 1 = second-newest, ...
    // Walk backward from the tail. O(k).
    // -------------------------------------------------------------------------
    HistoryNode* getFromNewest(int index) const {
        HistoryNode* cur = tail;
        while (cur && index > 0) { cur = cur->prev; --index; }
        return cur;
    }
};

#endif // ZONEHISTORY_H