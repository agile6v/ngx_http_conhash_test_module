About
========

This module for test the [ngx_consistent_hash][] api,  which is based on [ngx_consistent_hash][] module.


Configuration
========

conhash_test
----


* **syntax**:  `conhash_test`
* **default**: `-`
* **context**: `location`

Turns on this module.

conhash_test_zone
----


* **syntax**: `conhash_test_zone keys_zone=name:size [vnodecnt=count]`
* **default**: `-`
* **context**: `main`

Sets the share memory name、share memory size and vnode count of the consistent hash.

If you explicitly specifly the vnodecnt, it cannot be more than 10000. By default, vnodecnt is set to 100.

NOTE: If you want to use the [ngx_consistent_hash][] module must be defined in a similar directive.

Sample Config
========

```bash
worker_processes  4;

events {
    worker_connections  1024;
}

http {
    include       mime.types;
    default_type  application/octet-stream;
    
    conhash_test_zone keys_zone=conhash:5m;

    server {
        listen       80;
        server_name  localhost;

        location /conhash {
            conhash_test;
        }
    }
}
```

Testing
========

```bash
add:       curl -s "http://127.0.0.1/conhash?cmd=1&value=nodeA"
del:       curl -s "http://127.0.0.1/conhash?cmd=2&value=nodeA"
search:    curl -s "http://127.0.0.1/conhash?cmd=3&value=agile6v"
traverse:  curl -s "http://127.0.0.1/conhash?cmd=4"
clear:     curl -s "http://127.0.0.1/conhash?cmd=5"

### add node A
$ curl -s "http://127.0.0.1/conhash?cmd=1&value=A"
Add node successfully!

### add node B
$ curl -s "http://127.0.0.1/conhash?cmd=1&value=B"
Add node successfully!

### traverse vnode
$ curl -s "http://127.0.0.1/conhash?cmd=4" -o vnode.txt
$ cat vnode.txt

### search node
$ curl -s "http://127.0.0.1/conhash?cmd=3&value=agile6v"
agile6v(317471105) is in the node B(B-0599, 321124292)

### del node B
$ curl -s "http://127.0.0.1/conhash?cmd=2&value=B"
Delete node successfully!

### search node
$ curl -s "http://127.0.0.1/conhash?cmd=3&value=agile6v"
agile6v(317471105) is in the node A(A-0442, 322085099)

### add node B
$ curl -s "http://127.0.0.1/conhash?cmd=1&value=B"
Add node successfully!

### search node
$ curl -s "http://127.0.0.1/conhash?cmd=3&value=agile6v"
agile6v(317471105) is in the node B(B-0599, 321124292)

### clear all nodes
$ curl -s "http://127.0.0.1/conhash?cmd=5"

```

See also
========
* [ngx_consistent_hash][]

[ngx_consistent_hash]: https://github.com/agile6v/ngx_consistent_hash

