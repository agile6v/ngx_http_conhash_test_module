About
========

This module for test the [ngx_consistent_hash][] api,  which is based on ngx_consistent_hash module.


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
NOTE: If you want to use the [ngx_consistent_hash][] module must be defined in a similar directive.

Testing
========

```bash
add:       curl -s "http://127.0.0.1/conhash?key=1&value=nodeA"
del:       curl -s "http://127.0.0.1/conhash?key=2&value=nodeA"
search:    curl -s "http://127.0.0.1/conhash?key=3&value=agile6v"
traverse:  curl -s "http://127.0.0.1/conhash?key=4"

### add node A
$ curl -s "http://127.0.0.1/conhash?key=1&value=A"
Add node successfully!

### add node B
$ curl -s "http://127.0.0.1/conhash?key=1&value=B"
Add node successfully!

### traverse vnode
$ curl -s "http://127.0.0.1/conhash?key=4" -o vnode.txt
$ cat vnode.txt

### search node
$ curl -s "http://127.0.0.1/conhash?key=3&value=agile6v"
agile6v(317471105) is in the node B(B-0599, 321124292)

### del node B
$ curl -s "http://127.0.0.1/conhash?key=2&value=B"
Delete node successfully!

### search node
$ curl -s "http://127.0.0.1/conhash?key=3&value=agile6v"
agile6v(317471105) is in the node A(A-0442, 322085099)

### add node B
$ curl -s "http://127.0.0.1/conhash?key=1&value=B"
Add node successfully!

### search node
$ curl -s "http://127.0.0.1/conhash?key=3&value=agile6v"
agile6v(317471105) is in the node B(B-0599, 321124292)

```

See also
========
* [ngx_consistent_hash][]

[ngx_consistent_hash]: https://github.com/agile6v/ngx_consistent_hash

