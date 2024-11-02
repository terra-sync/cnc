FROM debian:latest as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    gcc make \
    libcurl4-openssl-dev libpq-dev libinih-dev automake \
    autoconf git libc6-dev curl pkg-config libssl-dev libcunit1-dev

RUN curl https://sh.rustup.rs -sSf | sh -s -- -y

# Add Rust to the PATH
ENV PATH="/root/.cargo/bin:${PATH}"

WORKDIR /app

COPY . .

RUN chmod +x scripts/build-rust-libs.sh
RUN ./autogen.sh && ./configure
RUN make

FROM debian:latest

# Install runtime libraries
RUN apt-get update && apt-get install -y \
    libpq5 libcurl4 libinih-dev libcunit1-dev

COPY --from=builder /app/cnc /cnc
COPY --from=builder /app/rust/email/target/debug/libemail.so rust/email/target/debug/libemail.so

# Set the binary as the entrypoint of the container
ENTRYPOINT ["/cnc"]
