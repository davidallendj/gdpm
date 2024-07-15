FROM cgr.dev/chainguard/wolfi-base

RUN apk add --no-cache tini

# run as nobody
USER 65534:65534


# build the binary then copy into container
RUN bin/compile.sh --link
RUN bin/compile.sh --all

COPY build/gdpm.static /gdpm

CMD ["/gdpm"]

ENTRYPOINT [ "/sbin/tini", "--" ]