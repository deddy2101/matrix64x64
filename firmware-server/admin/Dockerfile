FROM node:20-alpine

WORKDIR /app

# Install dependencies
COPY package*.json ./
RUN npm ci --only=production

# Copy app files
COPY . .

# Create binaries directory if not exists
RUN mkdir -p /app/binaries

EXPOSE 3000

CMD ["node", "server.js"]
