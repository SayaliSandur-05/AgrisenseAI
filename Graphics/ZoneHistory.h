// =============================================================================
// ZoneHistory.h  --  AgriSense AI  --  Programming Lab (Data Layer)
// -----------------------------------------------------------------------------
// Doubly-linked list of maintenance/event entries, one per zone.
//  - insertEvent()  : append at tail (chronological order)
//  - getOldest()    : head  (oldest-first traversal)
//  - getNewest()    : tail  (newest-first traversal -- our "reverseText" view)
//  - getFromNewest(): walk backwards by index, used by the History UI
//
// Owns its nodes; destructor frees them.
// =============================================================================
#ifndef ZONEHISTORY_H
#define ZONEHISTORY_H

#include <cstring>
#include <cstdio>

// ---------------- Node ----------------
struct HistoryNode {
    char category[48];
    char date[32];
    char detail[64];
    char note[64];
    HistoryNode* prev;
    HistoryNode* next;

    HistoryNode() : prev(NULL), next(NULL) {
        category[0] = date[0] = detail[0] = note[0] = '\0';
    }
};

// ---------------- List ----------------
class ZoneHistory {
private:
    HistoryNode* head;   // oldest
    HistoryNode* tail;   // newest
    int          count;

public:
    ZoneHistory() : head(NULL), tail(NULL), count(0) {}

    ~ZoneHistory() {
        HistoryNode* cur = head;
        while (cur) {
            HistoryNode* nxt = cur->next;
            delete cur;
            cur = nxt;
        }
    }

    // Append one event at the tail (chronological).
    void insertEvent(const char* category, const char* date,
                     const char* detail,   const char* note) {
        HistoryNode* node = new HistoryNode();

        std::strncpy(node->category, category ? category : "", sizeof(node->category) - 1);
        std::strncpy(node->date,     date     ? date     : "", sizeof(node->date)     - 1);
        std::strncpy(node->detail,   detail   ? detail   : "", sizeof(node->detail)   - 1);
        std::strncpy(node->note,     note     ? note     : "", sizeof(node->note)     - 1);

        if (!tail) {
            head = tail = node;           // first element
        } else {
            tail->next = node;
            node->prev = tail;
            tail       = node;
        }
        ++count;
    }

    // Oldest-first traversal entry point
    HistoryNode* getOldest() const { return head; }
    // Newest-first traversal entry point  (a.k.a. reverseText view)
    HistoryNode* getNewest() const { return tail; }

    int  getCount() const { return count; }
    bool isEmpty()  const { return count == 0; }

    // 0 = newest, 1 = second-newest, ... returns NULL if index out of range.
    HistoryNode* getFromNewest(int index) const {
        HistoryNode* cur = tail;
        while (cur && index > 0) { cur = cur->prev; --index; }
        return cur;
    }
};

#endif // ZONEHISTORY_H