CXX = g++
CXXFLAGS = -O2 -Wall
CC = gcc
CFLAGS = -O2 -Wall -std=c99 -fPIC
LDFLAGS = -lsqlite3
BIN = bin

MEDIATION_OBJS = mediation/capacity.o mediation/status.o mediation/circuit_counter.o mediation/store.o mediation/locations.o
# the rules themselves are one C file, shared with vantage-telco. see
# billing/rules/README.
RULES_SRC = billing/rules/billing_rules.c
RULES_OBJ = billing/rules/billing_rules.o
BILLING_OBJS = billing/accounts.o billing/invoice.o $(RULES_OBJ)

all: $(BIN)/mediation $(BIN)/inventory-api $(BIN)/billing-run $(BIN)/invoice-api $(BIN)/ipam \
     $(BIN)/libbillingrules.so

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

# the same rules as a shared object, for anything calling them through a
# foreign function interface.
$(BIN)/libbillingrules.so: $(RULES_SRC) billing/rules/billing_rules.h | $(BIN)
	$(CC) $(CFLAGS) -shared -o $@ $(RULES_SRC) -lm

$(RULES_OBJ) billing/rules/gen_vectors.o billing/invoice.o billing/rules_test.o: billing/rules/billing_rules.h

$(BIN)/gen-vectors: billing/rules/gen_vectors.o $(RULES_OBJ) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	$(BIN)/mediation --data data --db meridian.db

check: $(BIN)/billing-test vectors-check
	$(BIN)/billing-test

# regenerate the conformance vectors the other engines are held to
vectors: $(BIN)/gen-vectors
	$(BIN)/gen-vectors

# fail when the checked in vectors no longer match what the rules produce
vectors-check: $(BIN)/gen-vectors
	@tmp=$$(mktemp -d); \
	 $(BIN)/gen-vectors --rules $$tmp/rules.csv --invoices $$tmp/invoices.csv; \
	 status=0; \
	 diff -u billing/rules/conformance_vectors.csv $$tmp/rules.csv || status=1; \
	 diff -u billing/rules/invoice_vectors.csv $$tmp/invoices.csv || status=1; \
	 rm -rf $$tmp; \
	 if [ $$status -ne 0 ]; then echo "conformance vectors are stale, run make vectors"; fi; \
	 exit $$status

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
