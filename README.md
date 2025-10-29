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

Last reader unlocks, allowing writers

Writer requires exclusive access (blocks all readers and writers)

📊 System Architecture
text
Client (Browser)
    ↓ HTTP Requests
TCP Socket Server (Port 8080)
    ↓ Thread per connection
Request Handler
    ↓
Synchronization Layer (Semaphores/Mutexes)
    ↓
Shared Resource (5×8 Seat Array)
📁 Project Structure
text
.
├── server.c       # C backend with pthread and semaphores
├── index.html     # Interactive web interface
├── run.sh         # Build and launch script
└── README.md      # Documentation
🎯 Learning Outcomes
This project demonstrates:

Readers-Writers problem solution

Thread synchronization with semaphores

Mutex locks for critical sections

Multi-threaded HTTP server in C

Socket programming (TCP/IP)

Race condition prevention

Deadlock-free design

⚠️ Known Limitations
Reader Priority: Writers may experience starvation if readers continuously arrive

No Persistence: Data resets on server restart (in-memory only)

Local Only: Runs on localhost (not production-ready)

🤝 Contributing
Feel free to fork, submit issues, or create pull requests!

📝 License
MIT License - Free to use for educational purposes

👨‍💻 Author
Built as an educational project to demonstrate OS synchronization concepts.

⭐ Star this repo if you found it helpful!
