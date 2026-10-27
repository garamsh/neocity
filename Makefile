.PHONY: lint ci

lint:
	./check-contents-list.sh
	./check-object-names.sh
	./check-convention-index.sh
	./check-section-citations.sh
	./check-heading-prefixes.sh

ci: lint
