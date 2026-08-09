const std = @import("std");

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});

    const Target = std.Target.x86;
    const target = b.resolveTargetQuery(.{ .cpu_arch = .x86, .os_tag = .freestanding, .abi = .none, .cpu_features_add = Target.featureSet(&.{.soft_float}), .cpu_features_sub = Target.featureSet(&.{ .avx, .avx2, .sse, .sse2, .mmx }) });

    const kernel = b.addExecutable(.{ .name = "kernel.elf", .root_module = b.createModule(.{ .root_source_file = b.path("src/main.zig"), .target = target, .optimize = optimize, .code_model = .kernel }) });

    kernel.setLinkerScript(b.path("src/linker.ld"));

    b.installArtifact(kernel);
}
