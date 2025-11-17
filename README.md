# 🚀 ConcurMeet --- A Lightweight Google Meet--Style Backend

*A CSP Project using File I/O • Concurrent Programming • Pipes • Socket
Programming*

ConcurMeet is our attempt to implement the **backend architecture of a
video-conferencing platform like Google Meet**, but in a simplified,
system-programming--oriented way.\
The goal was **not UI**, but to demonstrate deep understanding of OS +
Networking concepts taught in our CSP course.

This project includes:

-   Multi-client real-time communication
-   Room-based meeting architecture (like Google Meet codes)
-   Concurrent server using threads
-   Message broadcasting
-   Join/leave notifications
-   Chat history saved per room using File I/O
-   Unique usernames
-   Modular, scalable system design

------------------------------------------------------------------------

# 🎯 Motivation

We wanted to go **beyond a simple chat application** and build something
that resembles the structure of Google Meet's backend:-
**rooms**, **participants**, **message passing**, **logging**,
**multiuser synchronization**, and **networked sockets**.

ConcurMeet demonstrates how real conferencing systems maintain multiple
isolated meeting environments with persistent logs and multi-user
concurrency.

------------------------------------------------------------------------

# ✨ Features

### ✔ Multiple Meeting Rooms

Clients can **create** or **join** rooms (like Google Meet codes).\
Users inside a room see only the messages from that room.

### ✔ Username Registration

Each client provides a unique username on join.

### ✔ Real-Time Messaging

Messages are broadcast only to members of the same room.

### ✔ Join/Leave Notifications

When a user joins or leaves a room, everyone inside the room gets
notified.

### ✔ Chat Logs (File I/O)

Every room has its own file:

    rooms/<RoomName>.log

All messages + join/leave events are written automatically.

### ✔ Full Concurrency

Server uses **POSIX threads** to handle multiple clients simultaneously.

### ✔ Scalable Server Design

Efficient structure using: - maps for username management\
- room-wise data structures\
- message broadcasting\
- persistent activity logs

------------------------------------------------------------------------

# 🧠 Concepts Used from CSP Course

### 🔹 **Socket Programming**

-   TCP sockets\
-   Binding, listening, accepting\
-   recv(), send(), shutdown()

### 🔹 **Concurrent Programming**

-   pthreads\
-   Thread-per-client model\
-   Shared data structures with synchronization

### 🔹 **File I/O**

-   Append logs per room\
-   Persistent message storage

### 🔹 **Pipes / System Calls**

(extendable; available for examiner demonstration)

------------------------------------------------------------------------

# 📦 Project Structure

    ConcurMeet/
    │
    ├── server.cpp        # Main server with rooms + logs + concurrency
    ├── client.cpp        # Client program
    └── rooms/            # Auto-created room logs
          └── <room>.log

------------------------------------------------------------------------

# ▶ How to Use

### **1️⃣ Compile**

``` bash
g++ server.cpp -o server -pthread
g++ client.cpp -o client -pthread
```

### **2️⃣ Start Server**

``` bash
./server
```

### **3️⃣ Start Clients (in multiple terminals)**

``` bash
./client
```

### **4️⃣ Enter username**

Each username must be unique.

### **5️⃣ Join or create a room**

Clients will be prompted:

    Enter room name to join or create:

### **6️⃣ Start chatting!**

Messages appear only to users in the same room, just like Google Meet
rooms.

------------------------------------------------------------------------

# 📚 How to Verify Chat Logs

After chatting in a room named "coding":

Check:

``` bash
cat rooms/coding.log
```

You will see:

    [JOIN] Vedant entered room coding
    [MESSAGE] Vedant: Hello everyone!
    [LEAVE] Vedant left room coding

------------------------------------------------------------------------

# 🚀 Future Improvements (Already Planned)

-   Private messaging inside rooms
-   Admin/host controls
-   Password-protected rooms
-   Distributed server support
-   Voice/video packet simulation
-   User blocking/unblocking

------------------------------------------------------------------------

# 👨‍💻 Authors

-   Shrey Patel
-   Devarshi Patel
-   Vedant Shah
-   Ishit Shripal
-   Kavya Halani

------------------------------------------------------------------------

# 🏁 Final Notes

This project goes **far beyond** a basic client-server chat.\
It demonstrates how we can create a **Google Meet--inspired meeting
backend** using pure system programming fundamentals:

> **rooms**, **participants**, **real-time broadcasting**,
> **concurrency**, and **persistent history**.



