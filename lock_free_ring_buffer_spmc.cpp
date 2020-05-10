#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<atomic>
using namespace std;

/*struct AtomicIndex {   
    int index = 0;
    uint32_t counter = 0;
};

Atomic_Ptr make_ptr(void* ptr, uint16_t counter) {
    Atomic_Ptr output;
    output.ptr = input;
    output.counter[3] = counter;
    return output;
}

Atomic_Ptr inc_ptr(Atomic_Ptr input) {
    input.counter[3]++;
    return input;
}

void* get_ptr(Atomic_Ptr input) {
    input.counter[3] = 0;
    return input.ptr;
}*/

template <class T>
class LockFreeBoundedBuffer {
    struct AtomicIndex {
    int index = 0;
    uint32_t counter = 0;
    };

    private:
    int capacity;
    T* buffer;
    int head;
    std::atomic<AtomicIndex> tail;
    sem_t items;
    sem_t slots;
    pthread_mutex_t lock;

    public:
    LockFreeBoundedBuffer(int size) {
        capacity = size;
        buffer = new T[size];
        head = 0;
        tail = AtomicIndex();
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
        //pthread_mutex_lock(&lock);
        AtomicIndex observed_tail;
        AtomicIndex new_tail;
        T ret_val;
        do {
            observed_tail = tail.load(std::memory_order_relaxed);
            int index = observed_tail.index;
            ret_val = buffer[index];
            new_tail.index = (index + 1) % capacity;
            new_tail.counter = observed_tail.counter + 1;
        } while (!tail.compare_exchange_weak(observed_tail, new_tail, 
                std::memory_order_release,
                std::memory_order_relaxed));
        //pthread_mutex_unlock(&lock);
        sem_post(&slots);
        return ret_val;
    }
    
};

LockFreeBoundedBuffer<int> temp_buf(10);
long t_array[100];

//std::atomic<AtomicIndex> atest;

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
    bool test = false;
    
    //test = std::atomic<AtomicIndex>{}.is_lock_free();

    if (test) cout << "YEEEET\n";
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

