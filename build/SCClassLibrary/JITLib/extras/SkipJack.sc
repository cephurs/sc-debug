SkipJack {

	classvar <>verbose = true, <all, <defaultClock;
	var updateFunc, <>dt, <>stopTest, <name, <clock, <task, restartFunc;

	*new { arg updateFunc, dt = 0.2, stopTest = false, name = "anon", clock, autostart=true;
		^super.newCopyArgs(updateFunc, dt, stopTest, name, clock).init( autostart );
	}

	*initClass {
		this.stopAll;
		all = IdentitySet[];
		defaultClock = AppClock;
	}

	*stop { |name| all.do { |skip| if (skip.name == name) { skip.stop } }; }

	*stopAll { all.do(_.stop).clear; }

	init { |autostart=true|
		task = Task ({
			if( verbose )	{ ("SkipJack" + name + "starts.").postcln };
				while { dt.value.wait; stopTest.value.not } { updateFunc.value(this) };
				this.stop;
			}, clock ? defaultClock);
		if ( autostart, { this.start } );
	}

	cmdPeriod {
		task.stop.play;
		if( verbose ) { ("SkipJack" + name + "is back up.").postcln };
	}

	start {
		task.stop.play;
		all.add(this);
		CmdPeriod.add(this);
		if( verbose ) { ("SkipJack" + name + "started.").postcln };
	}

	play { this.start }

	stop {
		task.stop;
		all.remove(this);
		CmdPeriod.remove(this);
		if( verbose ) { ("SkipJack" + name + "stopped.").postcln };
	}
}

Watcher : SkipJack {
	*new { arg name = "anon", updateFunc, dt=0.2, stopTest = false;
		"Watcher is only for backward compatibility, use SkipJack!".postln;
		^super.newCopyArgs(updateFunc, dt, stopTest, name).init.start;
	}
}
