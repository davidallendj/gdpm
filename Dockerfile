FROM cgr.dev/chainguard/wolfi-base

RUN apk add --no-cache tini

USER 65534:65534

COPY build/gdpm.static /gdpm

CMD ["/gdpm"]

ENTRYPOINT [ "/sbin/tini", "--" ]