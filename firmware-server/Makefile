.PHONY: build up down restart logs add-version generate-manifest clean help

# Variabili
COMPOSE=docker-compose
BINARIES_DIR=./binaries

help: ## Mostra questo help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

build: ## Build Docker image
	$(COMPOSE) build

up: ## Avvia il server
	$(COMPOSE) up -d
	@echo "✓ Server avviato su http://localhost:8080"
	@echo "✓ Visita http://localhost:8080 per vedere le versioni disponibili"

down: ## Ferma il server
	$(COMPOSE) down

restart: ## Riavvia il server
	$(COMPOSE) restart

logs: ## Mostra i log
	$(COMPOSE) logs -f

generate-manifest: ## Genera manifest.json
	@echo "Generazione manifest.json..."
	@bash scripts/generate_manifest.sh $(BINARIES_DIR)

add-version: ## Aggiungi nuova versione (es: make add-version VERSION=1.2.0+5)
	@if [ -z "$(VERSION)" ]; then \
		echo "❌ Specifica VERSION (es: make add-version VERSION=1.2.0+5)"; \
		exit 1; \
	fi
	@mkdir -p $(BINARIES_DIR)/$(VERSION)
	@echo "✓ Directory creata: $(BINARIES_DIR)/$(VERSION)"
	@echo "➡️  Copia il file firmware.bin in questa directory"
	@echo "➡️  Poi esegui: make generate-manifest"

clean: ## Rimuovi container e immagini
	$(COMPOSE) down -v --rmi local
	@echo "✓ Pulizia completata"

install-deps: ## Installa dipendenze locali (per generare manifest)
	@command -v jq >/dev/null 2>&1 || { echo "Installing jq..."; brew install jq; }
	@echo "✓ Dipendenze installate"
