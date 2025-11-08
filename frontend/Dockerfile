# Use a Node.js Alpine image for the builder stage
FROM oven/bun:1 AS builder
WORKDIR /app
COPY package*.json bun.lock ./
RUN bun install --frozen-lockfile
COPY . .
RUN bunx --bun vite build

# Use another Node.js Alpine image for the final stage
FROM oven/bun:1
WORKDIR /app
COPY --from=builder /app/build build/
COPY --from=builder /app/node_modules node_modules/
COPY package.json .
EXPOSE 3000
ENV NODE_ENV=production
CMD [ "bun", "run", "./build/index.js" ]
