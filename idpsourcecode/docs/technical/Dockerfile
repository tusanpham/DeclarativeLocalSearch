FROM debian:latest
MAINTAINER krr@cs.kuleuven.be

WORKDIR /root/

RUN apt-get update && \
    apt-get install -y libgmp3-dev  && \
    apt-get install -y locales

RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

ENV PATH=/root/idp/bin:/root/idp-ide:$PATH

COPY idp-3.6.0 idp-install

COPY idp-ide idp-ide

RUN ln -s /root/idp-install/bin/idp /usr/bin/idp

WORKDIR /root/idp

VOLUME /root/idp

EXPOSE 4004

CMD webID