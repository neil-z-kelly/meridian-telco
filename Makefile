CXX = g++
CC = cc
CXXFLAGS = -O2 -Wall
CFLAGS = -O2 -Wall
LDFLAGS = -lsqlite3
BIN = bin

# the shared billing rule library. meridian links it, vantage vendors the same
# source and dlopens $(BIN)/libtelcorules.so through ctypes.
RULES_SRC = billing/rules/telco_rules.c
RULES_OBJ = billing/rules/telco_rules.o
VECTORS = billing/rules/conformance/vectors.json

MEDIATION_OBJS = mediation/capacity.o mediation/status.o mediation/circuit_counter.o mediation/store.o mediation/locations.o
BILLING_OBJS = $(RULES_OBJ) billing/accounts.o billing/invoice.o

all: $(BIN)/mediation $(BIN)/inventory-api $(BIN)/billing-run $(BIN)/invoice-api $(BIN)/ipam \
     $(BIN)/libtelcorules.so

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

$(BIN)/libtelcorules.so: $(RULES_SRC) billing/rules/telco_rules.h | $(BIN)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(RULES_SRC) -lm

$(BIN)/rules-vectors: billing/rules/gen_vectors.o $(RULES_OBJ) | $(BIN)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	$(BIN)/mediation --data data --db meridian.db

check: $(BIN)/billing-test vectors-check
	$(BIN)/billing-test

# conformance vectors for consumers that cannot link the library (the java
# report module). always generated from telco_rules.c, never edited by hand.
vectors: $(BIN)/rules-vectors
	@mkdir -p $(dir $(VECTORS))
	$(BIN)/rules-vectors > $(VECTORS)

vectors-check: $(BIN)/rules-vectors
	@$(BIN)/rules-vectors > $(VECTORS).new
	@diff -u $(VECTORS) $(VECTORS).new || { \
	  rm -f $(VECTORS).new; \
	  echo "$(VECTORS) is stale: run make vectors and commit the result"; \
	  exit 1; }
	@rm -f $(VECTORS).new
	@echo "conformance vectors up to date"

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
