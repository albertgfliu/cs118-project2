# CS118 Project 2

Some quick notes for the grader

My client and server will keep trying to connect with no maximum limit on the number of tries, according to the TA at this piazza post: https://piazza.com/class/imch21ygvawhj?cid=352

The sequence numbers I chose were 0 through 30719, inclusive. This corresponds to 30720 bytes of data, since the project specifications say that the maximum sequence number should correspond to 30 Kbytes (30720 bytes). This implies 0 through 30719 inclusive.

I randomized the sequence numbers that the client and server begin at.

I did not finish implementing congestion control. The congestion control window will remain constant at 1024 bytes. However, it still succesfully transmits the file in lossy cases with re-ordered and delayed packets.


