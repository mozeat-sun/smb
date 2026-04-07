#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

perl - "$ROOT_DIR" <<'PERL'
use strict;
use warnings;

my ($root) = @ARGV;
my @roots = qw(src inc tests examples);
my @files;
for my $sub (@roots) {
    my $dir = "$root/$sub";
    next unless -d $dir;
    open my $fh, '-|', 'find', $dir, '-type', 'f', '(', '-name', '*.c', '-o', '-name', '*.h', ')' or die $!;
    while (my $line = <$fh>) {
        chomp $line;
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

sub parse_fn_name {
    my ($sig) = @_;
    my $norm = $sig;
    $norm =~ s/\s+/ /g;
    if ($norm =~ /([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*\{\s*$/) {
        return $1;
    }
    return "function";
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

for my $file (@files) {
    open my $fh, '<', $file or die "open $file: $!";
    my @lines = <$fh>;
    close $fh;

    my @out;
    my $in_doc = 0;

    for (my $i = 0; $i <= $#lines; $i++) {
        my $line = $lines[$i];
        my $t = trim($line);

        if (!$in_doc && $t =~ m{^/\*\*}) { $in_doc = 1; }
        if ($in_doc) {
            push @out, $line;
            if ($t =~ m{\*/\s*$}) { $in_doc = 0; }
            next;
        }

        if ($t eq '' || $t =~ /^#/ || $t =~ m{^//}) {
            push @out, $line;
            next;
        }

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
            my $k = $#out;
            while ($k >= 0 && trim($out[$k]) eq '') { $k--; }
            my $has_comment = 0;
            if ($k >= 0 && trim($out[$k]) =~ m{\*/\s*$}) {
                while ($k >= 0) {
                    my $x = trim($out[$k]);
                    if ($x =~ m{^/\*\*}) { $has_comment = 1; last; }
                    last if $x !~ m{^/\*|^\*|\*/$};
                    $k--;
                }
            }

            if (!$has_comment) {
                my $fn = parse_fn_name($sig);
                push @out, "/**\n";
                push @out, " * @brief Function $fn.\n";
                push @out, " */\n";
            }
        }

        for my $idx ($i .. $j) {
            push @out, $lines[$idx];
        }
        $i = $j;
    }

    open my $ofh, '>', $file or die "write $file: $!";
    print {$ofh} @out;
    close $ofh;
}
PERL

echo "Added missing function comments where needed."
