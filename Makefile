CXX = g++
CXXFLAGS = -O2 -Wall
CC = gcc
CFLAGS = -O2 -Wall -std=c99 -pedantic
LDFLAGS = -lsqlite3
BIN = bin
VECTORS = billing/rules/conformance_vectors.json

MEDIATION_OBJS = mediation/capacity.o mediation/status.o mediation/circuit_counter.o mediation/store.o mediation/locations.o
RULES_OBJS = billing/rules/telco_rules.o
BILLING_OBJS = $(RULES_OBJS) billing/accounts.o billing/invoice.o

all: $(BIN)/mediation $(BIN)/inventory-api $(BIN)/billing-run $(BIN)/invoice-api $(BIN)/ipam

$(BIN):
	mkdir -p $(BIN)

$(BIN)/mediation: $(MEDIATION_OBJS) mediation/ingest.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN)/inventory-api: $(MEDIATION_OBJS) inventory-api/main.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN)/billing-run: $(BILLING_OBJS) billing/run_billing.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN)/invoice-api: $(BILLING_OBJS) billing/invoice-api/main.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN)/billing-test: $(BILLING_OBJS) billing/rules_test.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN)/ipam: network/ipam.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BIN)/rules-vectors: $(RULES_OBJS) billing/rules/gen_vectors.o | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	$(BIN)/mediation --data data --db meridian.db

check: $(BIN)/billing-test vectors-check
	$(BIN)/billing-test

# the conformance vectors every estate is held to; vantage vendors this file
# alongside the rules source.
vectors: $(BIN)/rules-vectors
	$(BIN)/rules-vectors > $(VECTORS)

vectors-check: $(BIN)/rules-vectors
	@$(BIN)/rules-vectors > $(VECTORS).new; \
	 if cmp -s $(VECTORS) $(VECTORS).new; then rm -f $(VECTORS).new; \
	 else echo "$(VECTORS) is stale, run make vectors"; diff -u $(VECTORS) $(VECTORS).new | head -40; rm -f $(VECTORS).new; exit 1; fi

register: all
	$(BIN)/billing-run --period 2026-07

demo: all
	@echo "invoice-api on :8082, invoice register page on :8083"
	@$(BIN)/invoice-api & \
	 python3 -m http.server 8083 --directory dashboard & \
	 wait

clean:
	rm -f $(BIN)/* */*.o */*/*.o meridian.db

.PHONY: all run check vectors vectors-check register demo clean
