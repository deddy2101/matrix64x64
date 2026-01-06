FROM nginx:alpine

# Installa tools per generare manifest
RUN apk add --no-cache jq bash

# Copia script di generazione manifest
COPY scripts/generate_manifest.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/generate_manifest.sh

# Crea directory per binari
RUN mkdir -p /usr/share/nginx/html/binaries

# Esponi porta 80
EXPOSE 80

# Avvia nginx
CMD ["nginx", "-g", "daemon off;"]
