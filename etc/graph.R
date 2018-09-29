#!/usr/bin/env Rscript

options(scipen = 1000)

argv <- commandArgs(trailingOnly = T)

if (length(argv) != 1) {
	stop("Must supply input file.")
}

market <- read.table(argv[1])

require(dplyr)

buy <- market %>%
	filter(V1 == 'B') %>%
	select(V2, V3) %>%
	rename(p = V2, q = V3) %>%
	mutate(cum_q = NA) %>%
	arrange(desc(p)) %>%
	mutate(cum_q = cumsum(q))

buy <- rbind(buy, c(p = max(buy$p), q = 0, cum_q = 0))

sell <- market %>%
	filter(V1 == 'S') %>%
	select(V2, V3) %>%
	rename(p = V2, q = V3) %>%
	mutate(cum_q = NA)

sell <- rbind(sell, c(p = min(sell$p), q = 0, cum_q = 0)) %>%
	arrange(p) %>%
	mutate(cum_q = cumsum(q))

pdf("graph.pdf", width = 9, height = 6)

plot(buy$p ~ buy$cum_q, type = "S", col = "blue", xlim = c(0, max(buy$cum_q, sell$cum_q)), ylim = c(min(buy$p, sell$p), max(buy$p, sell$p)), xlab = "quantity", ylab = "price")

lines(sell$p ~ sell$cum_q, type = "S", col = "red", xlim = c(0, max(buy$cum_q, sell$cum_q)), ylim = c(min(buy$p, sell$p), max(buy$p, sell$p)), xlab = "quantity", ylab = "price")

dev.off()
