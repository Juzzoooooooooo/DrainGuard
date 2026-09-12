import {
  clamp,
  getWaterLevelState,
  getWaterStateColor,
} from '../src/utils/waterLevel';
import {colors} from '../src/theme';

describe('water level utilities', () => {
  it('classifies distance using the firmware threshold direction', () => {
    expect(getWaterLevelState(15, 20, 50)).toBe('critical');
    expect(getWaterLevelState(35, 20, 50)).toBe('warning');
    expect(getWaterLevelState(80, 20, 50)).toBe('normal');
  });

  it('maps states to the web app status colors', () => {
    expect(getWaterStateColor('critical')).toBe(colors.danger);
    expect(getWaterStateColor('warning')).toBe(colors.warning);
    expect(getWaterStateColor('normal')).toBe(colors.success);
  });

  it('clamps progress and servo values', () => {
    expect(clamp(-1, 0, 100)).toBe(0);
    expect(clamp(43, 0, 100)).toBe(43);
    expect(clamp(120, 0, 100)).toBe(100);
  });
});
