/**
 * Test: BaselineRateController AIMD behavior.
 *
 * Verifies:
 * - Initial rate equals configured rate
 * - OnCnpReceived causes multiplicative decrease
 * - Rate never drops below minimum
 * - TryRecovery causes additive increase after recovery interval
 * - Rate never exceeds initial rate
 */
#include "ns3/core-module.h"
#include "../model/baseline-rate-controller.h"

#include <iostream>
#include <cmath>

using namespace ns3;

int main (int argc, char *argv[])
{
  Simulator::Destroy (); // clean state

  int passed = 0;
  int total = 0;

  Ptr<BaselineRateController> rc = CreateObject<BaselineRateController> ();
  rc->SetInitialRate (DataRate ("10Gbps"));
  rc->SetMinRate (DataRate ("100Mbps"));
  rc->SetMultiplicativeDecreaseFactor (0.5);
  rc->SetAdditiveIncreaseStep (DataRate ("1Gbps"));
  rc->SetRecoveryInterval (MicroSeconds (100));

  // Test 1: Initial rate
  total++;
  if (rc->GetCurrentRate () == DataRate ("10Gbps"))
    {
      std::cout << "Test 1 (Initial Rate): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 1 (Initial Rate): FAIL (got " << rc->GetCurrentRate () << ")" << std::endl;
    }

  // Test 2: Multiplicative decrease
  total++;
  rc->OnCnpReceived (1);
  if (rc->GetCurrentRate () == DataRate ("5Gbps"))
    {
      std::cout << "Test 2 (MD after CNP): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 2 (MD after CNP): FAIL (got " << rc->GetCurrentRate () << ")" << std::endl;
    }

  // Test 3: Second decrease
  total++;
  rc->OnCnpReceived (1);
  if (rc->GetCurrentRate () == DataRate ("2500Mbps"))
    {
      std::cout << "Test 3 (Second MD): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 3 (Second MD): FAIL (got " << rc->GetCurrentRate () << ")" << std::endl;
    }

  // Test 4: Rate floor
  total++;
  for (int i = 0; i < 20; ++i)
    {
      rc->OnCnpReceived (1);
    }
  if (rc->GetCurrentRate ().GetBitRate () >= DataRate ("100Mbps").GetBitRate ())
    {
      std::cout << "Test 4 (Min Rate Floor): PASS" << std::endl;
      passed++;
    }
  else
    {
      std::cout << "Test 4 (Min Rate Floor): FAIL (got " << rc->GetCurrentRate () << ")" << std::endl;
    }

  // Test 5: Recovery — need to advance simulator time
  total++;
  // Reset to a known state
  Ptr<BaselineRateController> rc2 = CreateObject<BaselineRateController> ();
  rc2->SetInitialRate (DataRate ("10Gbps"));
  rc2->SetMinRate (DataRate ("100Mbps"));
  rc2->SetMultiplicativeDecreaseFactor (0.5);
  rc2->SetAdditiveIncreaseStep (DataRate ("1Gbps"));
  rc2->SetRecoveryInterval (MicroSeconds (100));

  // Cause a decrease at time 0
  rc2->OnCnpReceived (1); // rate = 5Gbps

  // Schedule a recovery check at time > recovery interval
  Simulator::Schedule (MicroSeconds (200), [&rc2, &passed, &total]() {
    rc2->TryRecovery (1);
    if (rc2->GetCurrentRate () == DataRate ("6Gbps"))
      {
        std::cout << "Test 5 (AI Recovery): PASS" << std::endl;
        passed++;
      }
    else
      {
        std::cout << "Test 5 (AI Recovery): FAIL (got " << rc2->GetCurrentRate () << ")" << std::endl;
      }
  });

  // Test 6: Rate ceiling
  Ptr<BaselineRateController> rc3 = CreateObject<BaselineRateController> ();
  rc3->SetInitialRate (DataRate ("10Gbps"));
  rc3->SetMinRate (DataRate ("100Mbps"));
  rc3->SetMultiplicativeDecreaseFactor (0.5);
  rc3->SetAdditiveIncreaseStep (DataRate ("1Gbps"));
  rc3->SetRecoveryInterval (MicroSeconds (10));

  // Decrease once
  rc3->OnCnpReceived (1); // 5Gbps

  // Schedule many recovery steps
  for (int i = 1; i <= 20; ++i)
    {
      Simulator::Schedule (MicroSeconds (10 * i + 500), [&rc3]() {
        rc3->TryRecovery (1);
      });
    }

  Simulator::Schedule (MicroSeconds (800), [&rc3, &passed, &total]() {
    total++;
    if (rc3->GetCurrentRate ().GetBitRate () <= DataRate ("10Gbps").GetBitRate ())
      {
        std::cout << "Test 6 (Rate Ceiling): PASS" << std::endl;
        passed++;
      }
    else
      {
        std::cout << "Test 6 (Rate Ceiling): FAIL (got " << rc3->GetCurrentRate () << ")" << std::endl;
      }
  });

  Simulator::Stop (MicroSeconds (1000));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << std::endl;
  std::cout << "BaselineRateController Test: " << passed << "/" << total << " passed" << std::endl;

  return (passed == total) ? 0 : 1;
}
