FROM debian:9

RUN apt-get update && apt-get install -y \
		autoconf \
		curl \
		g++ \
		libboost-program-options1.62-dev \
		libc-ares-dev \
		libcrypto++-dev \
		libcurl4-openssl-dev \
		libfreeimage-dev \
		libsodium-dev \
		libsqlite3-dev \
		libssl-dev \
		libtool \
		libuv1-dev \
		make \
		zlib1g-dev \
	&& rm -rf /var/lib/apt/lists/*

RUN cd /usr/src \
	&& curl -L https://github.com/meganz/sdk/archive/v3.1.9.tar.gz | tar xz \
	&& cd sdk-3.1.9 \
	&& ./autogen.sh \
	&& ./configure --with-libuv --disable-sync --disable-examples --disable-megacmd --prefix /usr \
	&& make \
	&& make install \
	&& make install DESTDIR=/usr/src/output

RUN cd /usr/src \
	&& curl -L https://github.com/connesc/megaproxy/archive/v0.2.0.tar.gz | tar xz \
	&& cd megaproxy-0.2.0 \
	&& CPPFLAGS=-I/usr/src/output/include LDFLAGS=-L/usr/src/output/lib make \
	&& install -D -m755 megaproxy /usr/src/output/usr/bin/megaproxy

FROM debian:9

RUN apt-get update && apt-get install -y \
		libboost-program-options1.62.0 \
		libc-ares2 \
		libcrypto++6 \
		libcurl3 \
		libfreeimage3 \
		libsodium18 \
		libsqlite3-0 \
		libssl1.1 \
		libuv1 \
		zlib1g \
	&& rm -rf /var/lib/apt/lists/*

COPY --from=0 /usr/src/output /

ENTRYPOINT ["megaproxy"]
