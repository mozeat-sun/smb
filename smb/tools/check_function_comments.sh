#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
RAW_OUT="${1:-/tmp/smb_missing_comments_raw.txt}"
SUMMARY_OUT="${2:-/tmp/smb_missing_comments_by_file.txt}"

: > "$RAW_OUT"

perl - "$ROOT_DIR" "$RAW_OUT" <<'PERL'
use strict;
use warnings;

my ($root, $raw_out) = @ARGV;
my @roots = qw(src inc);
my @files;
for my $sub (@roots) {
    my $dir = "$root/$sub";
    next unless -d $dir;
    open my $fh, '-|', 'find', $dir, '-type', 'f', '(', '-name', '*.c', '-o', '-name', '*.h', ')' or die $!;
    while (my $line = <$fh>) {
        chomp $line;
        $line =~ s#^\Q$root/\E##;
        push @files, $line;
    }
    close $fh;
}

sub trim {
    my ($s) = @_;
    $s =~ s/^\s+//;
    $s =~ s/\s+$//;
    return $s;
}

sub is_function_signature {
    my ($sig) = @_;
    my $norm = $sig;
    $norm =~ s/\s+/ /g;
    return 0 if $norm =~ /;\s*$/;
    return 0 unless $norm =~ /\(/ && $norm =~ /\)/ && $norm =~ /\)\s*\{\s*$/;
    return 0 if $norm =~ /^\s*(if|for|while|switch|return|else|do)\b/;
    return 0 if $norm =~ /^\s*typedef\b/;

    my ($head) = $norm =~ /^(.*?)\(/;
    return 0 unless defined $head;
    $head = trim($head);
    return 0 if $head eq '';
    return 0 if $head =~ /[=\[\]{}]/;
    return 0 if $head =~ /\b(sizeof|strcmp|strncmp|FD_ISSET|memcpy|memset)\b/;
    return 0 if $head =~ /->|\./;
    return 0 unless $head =~ /^[A-Za-z_][A-Za-z0-9_\s\*]*\b[A-Za-z_][A-Za-z0-9_]*$/;
    return 1;
}

open my $out, '>', $raw_out or die "open $raw_out: $!";
for my $rel (@files) {
    my $file = "$root/$rel";
    open my $fh, '<', $file or die "open $file: $!";
    my @lines = <$fh>;
    close $fh;

    my $in_doc = 0;
    for (my $i = 0; $i <= $#lines; $i++) {
        my $t = trim($lines[$i]);

        if (!$in_doc && $t =~ m{^/\*\*}) { $in_doc = 1; }
        if ($in_doc) {
            if ($t =~ m{\*/\s*$}) { $in_doc = 0; }
            next;
        }

        next if $t eq '' || $t =~ /^#/ || $t =~ m{^//};
        next if $t =~ /^(if|for|while|switch|return|else|do)\b/;

        my $start = $i;
        my $sig = $t;
        my $j = $i;
        my $saw_paren = ($sig =~ /\(/) ? 1 : 0;

        while ($j < $#lines) {
            last if $sig =~ /;\s*$/;
            last if $sig =~ /\)\s*\{\s*$/;
            $j++;
            my $n = trim($lines[$j]);
            next if $n eq '' || $n =~ m{^//};
            $sig .= ' ' . $n;
            $saw_paren = 1 if $n =~ /\(/;
            last if $n =~ /^\{/;
        }

        my $is_fn = $saw_paren && is_function_signature($sig);
        if ($is_fn) {
            my $k = $start - 1;
            while ($k >= 0 && trim($lines[$k]) eq '') { $k--; }
            my $has_comment = 0;
            if ($k >= 0 && trim($lines[$k]) =~ m{\*/\s*$}) {
                while ($k >= 0) {
                    my $x = trim($lines[$k]);
                    if ($x =~ m{^/\*\*}) { $has_comment = 1; last; }
                    last if $x !~ m{^/\*|^\*|\*/$};
                    $k--;
                }
            }

            if (!$has_comment) {
                print $out $rel . ':' . ($start + 1) . ':' . $sig . "\n";
            }
        }

        $i = $j;
    }
}
close $out;
PERL

awk -F: '{count[$1]++} END {for (f in count) print count[f], f}' "$RAW_OUT" | sort -nr > "$SUMMARY_OUT"

total="$(wc -l < "$RAW_OUT")"
echo "TOTAL_MISSING_FUNCTION_COMMENTS=$total"

if [[ "$total" -gt 0 ]]; then
  echo "Top files with missing comments:"
  head -n 20 "$SUMMARY_OUT"
    echo "WARNING: function comment compliance has gaps; continuing without failing tests."
    exit 0
fi

echo "Function comment compliance check passed."
