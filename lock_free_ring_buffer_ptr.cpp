#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<atomic>
using namespace std;

template <class T>
class LockFreeBoundedBuffer {
    struct AtomicIndex {
        int index = 0;
        uint32_t counter = 0;
    };
    union AtomicPtr {
        T* ptr;
        uint16_t counter[4];
    };
    /*
    inline AtomicPtr makePtr(T* ptr, uint16_t counter) {
        AtomicPtr output;
        output.ptr = ptr;
        output.counter[3] = counter;
        return output;
    }
    inline AtomicPtr setCounter(AtomicPtr input, uint16_t inputCounter) {
        input.counter[3] = inputCounter;
        return input;
    }
    inline AtomicPtr incPtr(AtomicPtr input) {
        input.counter[3]++;
        return input;
    }
    inline T* getPtr(AtomicPtr input) {
        input.counter[3] = 0;
        return input.ptr;
    }*/


    private:
    int capacity;
    std::atomic<AtomicPtr>* buffer;
    std::atomic<AtomicIndex> head;
    std::atomic<AtomicIndex> tail;
    sem_t items;
    sem_t slots;

    public:
    LockFreeBoundedBuffer(int size) {
        capacity = size;
        buffer = new std::atomic<AtomicPtr>[size]();
        head = AtomicIndex();
        tail = AtomicIndex();
        sem_init(&items, 0, 0);
        sem_init(&slots, 0, size);
    }

    void add (T* item) {
        sem_wait(&slots);
        int index;
        AtomicPtr newReference;
        newReference.ptr = item;
        AtomicIndex observedHead;
        AtomicIndex taggedHead;
        AtomicIndex newHead;
        AtomicPtr observedReference;

        do {
            observedHead = head;
            index = observedHead.index;
            observedReference = buffer[index];
            taggedHead = observedHead;
            taggedHead.counter++;
            newReference.counter[3] = 
                    observedReference.counter[3] + 1;
            newHead = taggedHead;
            newHead.index = (index + 1) % capacity;
        } while(!(head.compare_exchange_weak(
                        observedHead, taggedHead,
                        std::memory_order_release,
                        std::memory_order_relaxed) &&
                buffer[index].compare_exchange_weak(
                        observedReference, newReference,
                        std::memory_order_release,
                        std::memory_order_relaxed) &&
                head.compare_exchange_weak(
                        taggedHead, newHead,
                        std::memory_order_release,
                        std::memory_order_relaxed)));
        sem_post(&items);
    }

    T* remove () {
        sem_wait(&items);
        AtomicIndex observedTail;
        AtomicIndex newTail;
        AtomicPtr retPtr;
        do {
            observedTail = tail;
            newTail = observedTail;
            int index = observedTail.index;
            retPtr = buffer[index];
            newTail.index = (index + 1) % capacity;
            newTail.counter = observedTail.counter + 1;
        } while (!tail.compare_exchange_weak(
                observedTail, newTail,
                std::memory_order_release,
                std::memory_order_relaxed));
        sem_post(&slots);
        retPtr.counter[3] = 0;
        return retPtr.ptr;
    }
    
};

LockFreeBoundedBuffer<int> temp_buf(10);
long t_array[100];

//std::atomic<AtomicIndex> atest;

void* do_work(void* input) {
    long tid = (long) input;
    int tid_int = (int) tid;
    long local = 0;
    for (int i = 0; i < 1000; i++) {
        temp_buf.add((int*)i);
        local += (long) temp_buf.remove();
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

