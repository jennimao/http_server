# http_server

### team members
Sam Detor and Jenny Mao

### chunked response 
The server implements chunked responses using cursor and data length fields to keep track of how much has been written so far. 

### heartbeat monitoring 
The server has a /load endpoint that will return how many requests are currently being sent to the server to determine whether or not it can accept any more requests. 

### thread pool structure and asynchronous I/O using select 
The server has an event-driven symmetric concurrency structure implemented using kqueue in which there are k worker threads that all accept incoming client connections and process the requests.

### performance and benchmarking methodlogy 
So far, we have been testing using concurrent client executables. Further benchmarking to be done. 
