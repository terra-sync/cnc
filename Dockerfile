FROM alpine:latest as builder

# Install build dependencies
RUN apk add --no-cache gcc musl-dev make \
    curl-dev postgresql-dev inih-dev

WORKDIR /app
COPY . .

RUN make

FROM alpine:latest

# Install runtime libraries
RUN apk add --no-cache \
    libpq curl inih

COPY --from=builder /app/cnc /cnc

# Set the binary as the entrypoint of the container
ENTRYPOINT ["/cnc"]
