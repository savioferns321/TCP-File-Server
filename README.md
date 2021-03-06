The file 207httpd.c contains the source code for this project.

Makefile.hw2 is the makefile used to compile the source code.

"htdoc" is the folder that contains the files that are referenced by the server.
Note: htdoc also contains a file "favicon.ico" which is requested by some browsers during GET requests.

207httpd.c is a multithreaded TCP file server that is capable of handling multiple GET requests from multiple clients. 

1) Relevant files included : 

a) Makefile.hw2 -
 Used to compile and create executables for the programs. We can compile all the programs for the assignment by running the make command. The directions to run the makefile are as follows : 

- Open a terminal in Linux.
- Navigate to the folder in which the makefile and 207httpd.c program files is located.
- Type in 'make -f Makefile.hw2'
- All the executables for the .c programs will be created in the folder.

---------------------------------------------------------------------------------------------------
 
b) 207httpd.c
This is the server code. Run it as follows : 
./207httpd <port_no> <root_dir>

where port_no is the port number the server runs on and root_dir is the root directory where the files fetched by the server are located.
Eg :
./207httpd 9207 /home/htdoc
This server runs on port 9207 and it fetches files from the directory /home/htdoc/

Once the server is started, we can issue GET requests from a browser or from a Linux terminal by using the wget command.

Get requests from browser are made as follows : 
For the above server example:
http://localhost:9207/images/home.gif

This request fetches the file home.gif and displays it on the browser screen.
From terminal, make a GET request by entering the command:
wget http://localhost:9207/images/sjsu.jpg
The file will be downloaded to the root directory.

Notes : 
The server prints the client's IP address and port number to the console.
It also prints the complete GET request sent by the client to the console.
It also performs the following URL mappings : 
If the request is :

http://localhost:9207/

The file index.html residing at <root_dir>/index.html will be fetched.
If the request is :
http://localhost:9207/images/home.gif
The file residing at directory <root_dir>/img/sjsu.jpg will be fetched.

The server creates a new detached thread for every client request accepted by the server.
Also, if there are any files referenced by relative paths in the HTML file that is requested,
(eg: "images/sjsu.jpg")the server handles these as separate GET requests and sends them to the client.

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 
c) htdoc/ Folder
This folder contains the files that can be served by the server, to be used for testing. It contains
2 HTML files and a folder "img" which contains 2 image files. A favicon.ico file is requested by some browsers and is also stored in this folder for reference.




