// =============================================================================
// ZoneHistory.h  --  AgriSense AI  --  Programming Lab (Data Layer)
// -----------------------------------------------------------------------------
// Doubly-linked list of maintenance/event entries, one per zone.
// Rule of 3 added so std::vector<Zone> can copy/assign zones safely.
// =============================================================================
#ifndef ZONEHISTORY_H
#define ZONEHISTORY_H

#include <cstring>
#include <cstdio>

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

class ZoneHistory {
private:
    HistoryNode* head;
    HistoryNode* tail;
    int          count;

    void clear() {
        HistoryNode* cur = head;
        while (cur) { HistoryNode* nx = cur->next; delete cur; cur = nx; }
        head = tail = NULL; count = 0;
    }

public:
    ZoneHistory() : head(NULL), tail(NULL), count(0) {}

    // ---- Rule of 3: copy constructor ----
    ZoneHistory(const ZoneHistory& other) : head(NULL), tail(NULL), count(0) {
        for (HistoryNode* n = other.head; n; n = n->next)
            insertEvent(n->category, n->date, n->detail, n->note);
    }

    // ---- Rule of 3: assignment operator ----
    ZoneHistory& operator=(const ZoneHistory& other) {
        if (this == &other) return *this;
        clear();
        for (HistoryNode* n = other.head; n; n = n->next)
            insertEvent(n->category, n->date, n->detail, n->note);
        return *this;
    }

    ~ZoneHistory() { clear(); }

    void insertEvent(const char* category, const char* date,
                     const char* detail,   const char* note) {
        HistoryNode* node = new HistoryNode();
        std::strncpy(node->category, category ? category : "", sizeof(node->category) - 1);
        std::strncpy(node->date,     date     ? date     : "", sizeof(node->date)     - 1);
        std::strncpy(node->detail,   detail   ? detail   : "", sizeof(node->detail)   - 1);
        std::strncpy(node->note,     note     ? note     : "", sizeof(node->note)     - 1);

        if (!tail) head = tail = node;
        else { tail->next = node; node->prev = tail; tail = node; }
        ++count;
    }

    HistoryNode* getOldest() const { return head; }
    HistoryNode* getNewest() const { return tail; }
    int  getCount() const { return count; }
    bool isEmpty()  const { return count == 0; }

    HistoryNode* getFromNewest(int index) const {
        HistoryNode* cur = tail;
        while (cur && index > 0) { cur = cur->prev; --index; }
        return cur;
    }
};

#endif // ZONEHISTORY_H