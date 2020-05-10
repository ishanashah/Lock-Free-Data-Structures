#include<iostream>
#include<pthread.h>
#include<semaphore.h>
using namespace std;

template <class T>
class BoundedBuffer {
    private:
    int capacity;
    T* buffer;
    int head;
    int tail;
    sem_t items;
    sem_t slots;
    pthread_mutex_t lock;

    public:
    /*BoundedBuffer() {
        capacity = 10;
        buffer = new T[capacity];
        head = 0;
        tail = 0;
    }*/

    BoundedBuffer(int size) {
        capacity = size + 1;
        buffer = new T[size + 1];
        head = 0;
        tail = 0;
        sem_init(&items, 0, 0);
        sem_init(&slots, 0, size);
        pthread_mutex_init(&lock, 0);
    }

    void add (T item) {
        sem_wait(&slots);
        pthread_mutex_lock(&lock);
        buffer[head] = item;
        head = (head + 1) % capacity;
        pthread_mutex_unlock(&lock);
        sem_post(&items);
    }

    T remove () {
        sem_wait(&items);
        pthread_mutex_lock(&lock);
        T ret_val = buffer[tail];
        tail = (tail + 1) % capacity;
        pthread_mutex_unlock(&lock);
        sem_post(&slots);
        return ret_val;
    }
    
};

BoundedBuffer<int> temp_buf(10);
long t_array[100];



void* do_work(void* input) {
    long tid = (long) input;
    long local = 0;
    for (int i = 0; i < 1000; i++) {
        temp_buf.add(i);
        local += temp_buf.remove();
    }
    t_array[tid] = local;
    //cout << "tESTING\n";
    return NULL;
}

int main() {
    cout << "YEEEET\n";
    //BoundedBuffer<int> temp_buf(10);
    //cout << temp_buf.getCapacity() << "\n";
    /*
    temp_buf.add(1);
    temp_buf.add(3);
    temp_buf.add(7);

    cout << (int) temp_buf.remove() << "\n";
    cout << (int) temp_buf.remove() << "\n";
    cout << (int) temp_buf.remove() << "\n";
    */

    pthread_t thread_id[100];
    for (int i = 0; i < 100; i++) {
        pthread_create(&(thread_id[i]), NULL, do_work, (void*) i);
    }
    for (int i = 0; i < 100; i++) {
        pthread_join(thread_id[i], NULL);
    }
    long sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += t_array[i];
    }
    cout << "SUM IS: " << sum << "\n";

    sum = 0;
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 1000; j++) {
            sum += j;
        }
    }
    cout << "SUM IS: " << sum << "\n";
    cout << "SUCCESS!!!\n";
}

