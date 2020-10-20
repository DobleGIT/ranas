# ranas
Programa para aprender a manejar los procesos en linux
He añadido un pdf con el enunciado de la practica, pero basicamente consiste en que cada ranita es un proceso en linux y tienen que llega solas a la otra orilla. 


Para poder ejecutar el programa necesitais poder ejecutar aplicaciones de 32bits en vuestro linux para ello necesitais seguir los siguientes pasos:
Con la orden dpkg --print-architecture, tienes que comprobar que realmente tienes un Linux de 64 bits. Debe aparecer, amd64.
Mete ahora la orden dpkg --print-foreign-architectures. Si entre la salida no aparece i386, debes teclear: sudo dpkg --add-architecture i386
Ahora necesitas tener las bibliotecas de 32 bits también instaladas. Lo lograras con: sudo apt-get install g++-multilib
Finalmente, podrás hacer una prueba para ver si todo funciona. Para ello escribe gcc -m32 batracios.c libbatracios.a -o batracios -lm
Te saldrán muchos warnings pero son todos de libbatracios.a
Para ejecutar el programa final teclea ./batracios.c y para finalizarlo control+c
