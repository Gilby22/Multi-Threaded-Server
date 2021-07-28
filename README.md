Test machine: csel-kh1250-13.cselabs.umn.edu<br>
Name: Declan McGraw, Caleb Twiggs, Noah Gilbertson<br>
X500: mcgra406, twigg015, gilb0360<br>
Group 110<br>
How to compile and run our program:<br>
    make<br>
    ./web_server 9000 ./testing 3 6 0 20 20<br>
    wget http://127.0.0.1:9000/image/jpg/29.jpg<br>
How our program works:<br>
    Our program mimics a web-server using POSIX threads. In this project, we use two different types of threads, namely dispatcher
    threads and worker threads. The purpose of the dispatcher threads is to repeatedly accept an incoming connection,
    read the client request from the connection, and place the request in a queue. The purpose of the worker threads is to monitor
    the request queue, retrieve requests and serve the request’s result back to the client.<br>
We attempted the Extra Credit A, but we could not get it working without breaking the rest of our project.<br>
We each contributed equally to the development of the project. Having worked together previously, we worked together nicely on both coding and debugging. Everyone contributed to coding and debugging, with Caleb also providing documentation.
