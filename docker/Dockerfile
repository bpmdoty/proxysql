FROM debian:11 as build1

RUN apt-get update && \
    apt-get install -y wget lsb-release gnupg apt-transport-https ca-certificates && \
    rm -rf /var/lib/apt/lists/*

FROM build1 as build2
COPY binaries/proxysql_2.2.0-debian11_amd64.deb /root
RUN apt-get install /root/proxysql_2.2.0-debian11_amd64.deb

ENTRYPOINT ["proxysql", "-f", "--idle-threads", "--reload", "-D", "/var/lib/proxysql"]

