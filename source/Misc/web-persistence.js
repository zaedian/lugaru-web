Module['preRun'] = Module['preRun'] || [];

Module['preRun'].push(function() {
    addRunDependency('lugaru-idbfs');

    function mkdirSafe(path) {
        try {
            FS.mkdir(path);
        } catch (e) {
            if (e.errno !== 20) {
                throw e;
            }
        }
    }

    function fileExists(path) {
        try {
            FS.stat(path);
            return true;
        } catch (e) {
            return false;
        }
    }

    function finishStartup() {
        removeRunDependency('lugaru-idbfs');
    }

    try {
        mkdirSafe('/home');
        mkdirSafe('/home/web_user');

        FS.mount(IDBFS, {
            autoPersist: true
        }, '/home/web_user');

        FS.syncfs(true, function(err) {
            if (err) {
                console.error('Failed to load Lugaru persistent storage:', err);
                finishStartup();
                return;
            }

            try {
                mkdirSafe('/home/web_user/.config');
                mkdirSafe('/home/web_user/.config/lugaru');
                mkdirSafe('/home/web_user/.local');
                mkdirSafe('/home/web_user/.local/share');
                mkdirSafe('/home/web_user/.local/share/lugaru');

                var migrated = false;
                var ls = window['localStorage'];

                var configPath =
                    '/home/web_user/.config/lugaru/config.txt';

                if (!fileExists(configPath)) {
                    var configData = ls.getItem('lugaru_config');

                    if (configData) {
                        FS.writeFile(configPath, configData);
                        migrated = true;
                        console.log(
                            'Migrated legacy Lugaru config to IndexedDB'
                        );
                    }
                }

                var usersPath =
                    '/home/web_user/.local/share/lugaru/users';

                if (!fileExists(usersPath)) {
                    var usersData = ls.getItem('lugaru_users');

                    if (usersData) {
                        try {
                            var binary = atob(usersData);
                            var bytes = new Uint8Array(binary.length);

                            for (var i = 0; i < binary.length; i++) {
                                bytes[i] = binary.charCodeAt(i);
                            }

                            if (bytes.length < 8) {
                                console.warn(
                                    'Legacy Lugaru users data is too short; leaving it untouched'
                                );
                            } else {
                                var accountCount =
                                    (bytes[0] << 24) |
                                    (bytes[1] << 16) |
                                    (bytes[2] << 8) |
                                    bytes[3];

                                if (accountCount < 0 || accountCount > 100) {
                                    console.warn(
                                        'Legacy Lugaru users data failed validation; leaving it untouched'
                                    );
                                } else {
                                    FS.writeFile(usersPath, bytes);
                                    migrated = true;
                                    console.log(
                                        'Migrated legacy Lugaru users to IndexedDB'
                                    );
                                }
                            }
                        } catch (e) {
                            console.warn(
                                'Could not migrate legacy Lugaru users data; leaving it untouched:',
                                e
                            );
                        }
                    }
                }

                if (migrated) {
                    FS.syncfs(false, function(syncErr) {
                        if (syncErr) {
                            console.error(
                                'Failed to persist migrated Lugaru data:',
                                syncErr
                            );
                        }

                        console.log(
                            'Lugaru persistent filesystem ready'
                        );
                        finishStartup();
                    });
                } else {
                    console.log(
                        'Lugaru persistent filesystem ready'
                    );
                    finishStartup();
                }
            } catch (e) {
                console.error(
                    'Failed to prepare Lugaru persistent storage:',
                    e
                );
                finishStartup();
            }
        });
    } catch (e) {
        console.error(
            'Failed to initialize Lugaru persistent storage:',
            e
        );
        finishStartup();
    }
});
