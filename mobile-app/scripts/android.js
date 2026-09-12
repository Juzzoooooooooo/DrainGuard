const {spawnSync} = require('child_process');
const path = require('path');

const command = process.argv[2];
const tasks = {
  build: ['assembleRelease'],
  install: ['installRelease'],
};

if (!tasks[command]) {
  console.error('Usage: node scripts/android.js <build|install>');
  process.exit(1);
}

const projectRoot = path.resolve(__dirname, '..');
const androidRoot = path.join(projectRoot, 'android');
const wrapper = process.platform === 'win32' ? 'gradlew.bat' : './gradlew';
const result = spawnSync(wrapper, tasks[command], {
  cwd: androidRoot,
  env: process.env,
  shell: process.platform === 'win32',
  stdio: 'inherit',
});

if (result.error) {
  console.error(result.error.message);
  process.exit(1);
}

if (result.status !== 0) {
  process.exit(result.status ?? 1);
}

if (command === 'build') {
  console.log(
    `Installable APK: ${path.join(
      androidRoot,
      'app',
      'build',
      'outputs',
      'apk',
      'release',
      'app-release.apk',
    )}`,
  );
}
