# TCP-UDP
# TCP-UDP

Send 200 MB of text file using TCP and UDP method.

## Compilation

Use the GCC (GNU Compiler) to compile.

```bash
gcc -o <name> unify.c
```

## Execution

Server should be executed first.

* TCP

  * Client

  ```bash
  ./<name> tcp send <ip> <port> <filename>
  ```

  * Server
  ```bash
  ./<name> tcp recv <ip> <port>
  ```

* UDP

  * Client
  ```bash
  ./<name> udp send <ip> <port> <filename>
  ```

  * Server
  ```bash
  ./<name> udp recv <ip> <port>
  ```


## Result

Test file used is 204 MB of repeated ```1234567890123456789``` string.

* TCP: Takes ± 10 seconds  
  ![Result of TCP] (https://github.com/BenedictusKent/TCP-UDP/blob/main/images/image1.png)

* UDP: Takes ± 80 seconds  
  ![Result of UDP] (https://github.com/BenedictusKent/TCP-UDP/blob/main/images/image2.png)

## License
[MIT](https://choosealicense.com/licenses/mit/)
