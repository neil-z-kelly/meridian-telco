CXX = g++
CXXFLAGS = -O2 -Wall
CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -lsqlite3
BIN = bin

MEDIATION_OBJS = mediation/capacity.o mediation/status.o mediation/circuit_counter.o mediation/store.o mediation/locations.o
RULES_OBJS = rules/telco_rules.o
BILLING_OBJS = $(RULES_OBJS) billing/accounts.o billing/invoice.o

all: $(BIN)/mediation $(BIN)/inventory-api $(BIN)/billing-run $(BIN)/invoice-api $(BIN)/ipam

$(BIN):
	mkdir -p $(BIN)

$(BIN)/mediation: $(MEDIATION_OBJS) mediation/ingest.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN)/inventory-api: $(MEDIATION_OBJS) inventory-api/main.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN)/billing-run: $(BILLING_OBJS) billing/run_billing.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

$(BIN)/invoice-api: $(BILLING_OBJS) billing/invoice-api/main.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

$(BIN)/billing-test: $(BILLING_OBJS) billing/rules_test.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

$(BIN)/shared-rules-test: $(RULES_OBJS) billing/shared_rules_test.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lm

$(BIN)/rules-vectors: $(RULES_OBJS) rules/gen_vectors.o | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(BIN)/ipam: network/ipam.o | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	$(BIN)/mediation --data data --db meridian.db

check: $(BIN)/billing-test $(BIN)/shared-rules-test
	$(BIN)/billing-test
	$(BIN)/shared-rules-test

vectors: $(BIN)/rules-vectors
	$(BIN)/rules-vectors > rules/conformance-vectors.json

register: all
	$(BIN)/billing-run --period 2026-07

demo: all
	@echo "invoice-api on :8082, invoice register page on :8083"
	@$(BIN)/invoice-api & \
	 python3 -m http.server 8083 --directory dashboard & \
	 wait

clean:
	rm -f $(BIN)/* */*.o */*/*.o meridian.db

.PHONY: all run check register demo vectors clean
