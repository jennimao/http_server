# http_server

### Team members
Sam Detor and Jenny Mao

### Chunked response 
The server implements chunked responses using cursor and data length fields to keep track of how much has been written so far. 

### Heartbeat monitoring 
The server has a /load endpoint that will return how many requests are currently being sent to the server to determine whether or not it can accept any more requests. 

### Thread pool structure and asynchronous I/O using select 
The server has an event-driven symmetric concurrency structure implemented using kqueue in which there are k worker threads that all accept incoming client connections and process the requests.

### Performance and benchmarking methodlogy 
<img width="728" alt="Screenshot 2023-12-14 at 1 06 05 AM" src="https://github.com/jennimao/http_server/assets/79879717/4c62b9b3-faa1-4500-8985-159013ca688c">

Max Throughput reached (in Mbps): 10.06296
We toggled the kMaxConnections, kMaxEvents and kThreadPoolSize. We ended up at the following setting: kMaxConnections = 500 kMaxEvents = 100; kThreadPoolSize = 4; With increased values of these variables, there was too much overhead for the server to handler and it slowed down transmission speed.
