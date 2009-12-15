package com.jezhumble.javasysmon;

public class MacOsXMonitor implements Monitor {

    private static Monitor monitor = null;

    static {
        if (System.getProperty("os.name").toLowerCase().equals("mac os x")) {
            System.loadLibrary("javasysmon");
            monitor = new MacOsXMonitor();
        }
    }

    public MacOsXMonitor() {
        if (monitor != null) {
            JavaSysMon.setMonitor(monitor);
            JavaSysMon.addSupportedConfig("Mac Os X (PPC, x86, X86_64)");
        }
    }

    public String osName() {
        return System.getProperty("os.name") + " " + System.getProperty("os.version");
    }

    public native float cpuUsage();
    public native long totalMemory();
    public native long freeMemory();
    public native long totalSwap();
    public native long freeSwap();
    public native int numCpus();
    public native long cpuFrequency();
}