FROM 1733116199/achal-base
RUN apt-get install -y --no-install-recommends iproute2 dnsutils
USER pi
EXPOSE 22
WORKDIR /home/pi/achal
COPY ["./ae/id_rsa.pub", "./ae/id_rsa.pub"]
RUN sudo chown pi /home/pi/achal && \
    mkdir -p /home/pi/achal/data /home/pi/achal/build &&\
    cat ./ae/id_rsa.pub > /home/pi/.ssh/authorized_keys &&\
    chmod 600 /home/pi/.ssh/authorized_keys
COPY ["./config", "./config"]
COPY ["./exp", "./exp"]
COPY ["./third-party", "./third-party"]
COPY ["./CMakeLists.txt", "./CMakeLists.txt"]
COPY ["./scripts", "./scripts"]
COPY ["./src", "./src"]
COPY ["./test", "./test"]
WORKDIR /home/pi/achal/build
RUN cmake -GNinja .. && ninja -j 4
WORKDIR /home/pi/achal
CMD sudo service ssh start && tail -f /dev/null
