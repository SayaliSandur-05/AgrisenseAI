#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>
#include <string>
#include "CropProfile.h"

using namespace std;

// ==========================================
// 1. SORTING: Bubble Sort for Crops
// ==========================================
 
inline void bubbleSortCrops(vector<CropProfile>& crops) {
    int n = crops.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (crops[j].name > crops[j + 1].name) {
                // Swap elements
                CropProfile temp = crops[j];
                crops[j] = crops[j + 1];
                crops[j + 1] = temp;
            }
        }
    }
}

// ==========================================
// 2. STACK: Navigation History (LIFO)
// ==========================================
// Used for the "Back" button.
class NavStack {
private:
    int arr[50];
    int top;
public:
    NavStack() { top = -1; }
    
    void push(int zoneId) {
        if (top < 49) arr[++top] = zoneId;
    }
    
    int pop() {
        if (top >= 0) return arr[top--];
        return -1; // Empty
    }
    
    bool isEmpty() { return top == -1; }
};

// ==========================================
// 3. QUEUE: Pending Alerts (FIFO)
// ==========================================

class AlertQueue {
private:
    int arr[50];
    int front, rear;
public:
    AlertQueue() { front = 0; rear = 0; }
    
    void enqueue(int zoneId) {
        // Simple check to prevent duplicates in the queue
        for (int i = front; i < rear; i++) {
            if (arr[i] == zoneId) return; 
        }
        if (rear < 50) arr[rear++] = zoneId;
    }
    
    int dequeue() {
        if (front < rear) return arr[front++];
        return -1; // Empty
    }
    
    int peek() {
        if (front < rear) return arr[front];
        return -1;
    }
    
    bool isEmpty() { return front == rear; }
};

#endif