🎭 Theater Ticket Booking System
A multi-threaded theater ticket booking system demonstrating the Readers-Writers synchronization problem using POSIX semaphores and mutex locks in C. Features a real-time interactive web interface for concurrent seat booking.

🚀 Features
Concurrent Read Access: Multiple users can view seats simultaneously

Exclusive Write Access: Only one user can book seats at a time

Real-time Updates: Live seating chart with instant feedback

Readers-Writers Solution: Classic OS synchronization problem implementation

Thread-Safe: Protected by semaphores and mutexes

Visual Feedback: Color-coded seats (green=available, red=booked)

Activity Logging: Real-time log of all operations

🛠️ Technology Stack
Backend: C with POSIX threads (pthreads)

Frontend: HTML5, CSS3, Vanilla JavaScript

Synchronization: Semaphores (sem_t) and Mutexes (pthread_mutex_t)

Networking: TCP sockets with HTTP server

📋 Prerequisites
GCC compiler

Linux/Ubuntu OS

pthread library

⚡ Quick Start
bash
chmod +x run.sh
./run.sh
Or manually:

bash
gcc -o server server.c -lpthread
./server
Then open http://localhost:8080 in your browser.

🎮 How to Use
Viewer Mode: Click to view available seats (multiple users allowed)

Booking Mode: Click "BOOK" to get exclusive booking lock

Select Seats: Click green seats to book them (turns red)

Release Lock: Click "CANCEL" to allow others to book

Test Concurrency: Open multiple browser tabs and see the synchronization in action!

🔐 Synchronization Mechanism
Semaphores:
rw_mutex: Controls exclusive write access to seat data

mutex_lock: Protects the read_count variable

Mutexes:
booking_mutex: Ensures only one user can enter booking mode

log_mutex: Thread-safe activity logging

Key Algorithm:
First reader locks out all writers

Last reader unlocks, al
