TARGET = rpotool
SRC_DIR = src

RUSTC = cargo
RUSTFLAGS = build --release

SRCS = $(wildcard $(SRC_DIR)/*.rs)

$(TARGET): $(SRCS)
	$(RUSTC) $(RUSTFLAGS)
	mv target/release/$(TARGET) .

clean:
	cargo clean
