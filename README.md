# Medis

A simple key-value store implementation with Redis-like protocol.

## Building

```bash
# Install dependencies (macOS)
brew install cunit

# Build
make

# Run tests
make test
```

## Usage

Start the server:
```bash
./medis
```

Connect using Redis CLI:
```bash
redis-cli -p 6379
```

## Project Structure

```
medis/
├── src/           # Source code
├── tests/         # Unit tests
└── docs/          # Documentation
```

## License

MIT License - see [LICENSE](LICENSE) for details.